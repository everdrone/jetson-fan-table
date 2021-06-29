#include <assert.h>
#include <getopt.h>

#include <algorithm>
#include <chrono>
#include <thread>

#include "atexit.h"
#include "config.h"
#include "defines.h"
#include "interpolate.h"
#include "jetson_clocks.h"
#include "log.h"
#include "parse_table.h"
#include "thermal.h"
#include "utils.h"

using std::string;
using std::vector;

void print_version_exit() {
  std::cout << PACKAGE_STRING << std::endl;
  exit(EXIT_SUCCESS);
}

void print_help_exit() {
  printf(
      "%s [options]\n"
      "    -h --help                       Show this help\n"
      "    -v --version                    Show version\n"
      "    -c --check                      Check configurations and permissions\n"
      "    -i --interval <int>             Interval in seconds (defaults to 2)\n"
      "    -t --enable-tach                Enable the fan tachometer for speed monitoring\n"
      "    -M --no-max-freq                Do not set CPU and GPU clocks\n"
      "    -A --no-average                 Use the highest measured temperature instead of\n"
      "                                    calculating the average\n"
      "    -s --ignore-sensors <string>    Ignore sensors that match a substring (case sensitive)\n"
      "       --debug                      Increase verbosity in syslog\n",
      argv0);
  exit(EXIT_SUCCESS);
}

enum options_enum {
  OPTION_DEBUG = 256,
};

typedef struct options_struct {
  bool help = false;
  bool version = false;
  bool check = false;
  bool use_highest = false;
  string substring = "PMIC";
  unsigned interval = 2;
} options_t;

int main(int argc, char* argv[]) {
  // set global program name
  argv0 = argv[0];

  /*
   * Parse options
   */
  options_t options_object;

  // clang-format off
  static const struct option long_options[] = {
    {"help",            no_argument,        NULL, 'h'},
    {"version",         no_argument,        NULL, 'v'},
    {"check",           no_argument,        NULL, 'c'},
    {"interval",        required_argument,  NULL, 'i'},
    {"enable-tach",     no_argument,        NULL, 't'},
    {"no-max-freq",     no_argument,        NULL, 'M'},
    {"no-average",      no_argument,        NULL, 'A'},
    {"ignore-sensors",  required_argument,  NULL, 's'},
    {"debug",           no_argument,        NULL, OPTION_DEBUG},
    {NULL,              0,                  NULL, 0}};
  // clang-format on

  int opt;
  while ((opt = getopt_long(argc, argv, "hvci:tMAs:", long_options, NULL)) >= 0) {
    switch (opt) {
      case 'h':
        options_object.help = true;
        break;
      case 'v':
        options_object.version = true;
        break;
      case 'c':
        options_object.check = true;
        break;
      case 't':
        enable_tach = true;
        break;
      case 'M':
        enable_max_freq = false;
        break;
      case 'A':
        options_object.use_highest = true;
        break;
      case 'i':
        try {
          options_object.interval = std::stoul(optarg);
        } catch (...) {
          daemon_log(LOG_ERR, "cannot parse argument `%s' for --interval", optarg);
          fprintf(stderr, "%s: cannot parse argument `%s' for --interval\n", argv0, optarg);
          exit(EXIT_FAILURE);
        }
        break;
      case 's':
        options_object.substring = optarg;
        break;
      case OPTION_DEBUG:
        enable_debug = true;
        break;
      // case '?':
      //   if (optopt == 'i')
      //     fprintf(stderr, "Option -%c requires an argument\n", optopt);
      //   else if (isprint(optopt))
      //     fprintf(stderr, "Unknown option `-%c'\n", optopt);
      //   else
      //     fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
      default:
        return -1;
    }
  }

  assert(opt);

  if (optind < argc) {
    daemon_log(LOG_ERR, "too many arguments");
    sprintf_stderr("%s: too many arguments", argv0);
    exit(EXIT_FAILURE);
  }

  // print help and exit
  if (options_object.help) {
    print_help_exit();
  }

  // print version and exit
  if (options_object.version) {
    print_version_exit();
  }

  // print version and exit
  if (options_object.check) {
    /*
     * FIXME: still gives segmentation fault
     *
     * probably because of invalid table file
     */
    // parse_table(TABLE_PATH, true);
    exit(EXIT_SUCCESS);
  }

  // Start logging
  daemon_log(LOG_INFO, "Starting fan control daemon...");

  // check if is first time running
  if (access(INITIAL_STORE_FILE, F_OK) == -1) {
    is_first_run = true;
  }

  // if fantable runs AFTER nvpmodel.service this should not be necessary
  remove_file(STORE_FILE);
  daemon_log(LOG_INFO, "saving state to: `%s'", is_first_run ? INITIAL_STORE_FILE : STORE_FILE);
  store_config(is_first_run ? INITIAL_STORE_FILE : STORE_FILE);

  register_exit_handler();

  // enbale tachomenter
  if (enable_tach) {
    debug_log("enabling tachometer");
    tach_state = read_file_int(TACH_ENABLE_PATH);
    write_file_int(TACH_ENABLE_PATH, 1);
  }

  debug_log("using interval of %d seconds", options_object.interval);

  std::chrono::seconds interval(options_object.interval);

  unsigned temperature;
  int temperature_old = -1;
  unsigned pwm;

  unsigned clocks_wait = MAX_FREQ_WAIT / options_object.interval;

  if (options_object.use_highest) {
    debug_log("using highest measured temperature");
  }

  /*
   * read table file
   */
  debug_log("using table file `%s'", TABLE_PATH);

  vector<coord_t> table_config = parse_table(TABLE_PATH, true);

  // for (const auto& row : table_config) {
  //   std::cout << row.x << "->" << row.y << std::endl;
  // }

  if (table_config.size() < 1) {
    // TODO: handle invalid (empty?) table file
    daemon_log(LOG_ERR, "empty table configuration, possibly a parse error at `%s'", TABLE_PATH);
    sprintf_stderr("%s: empty table configuration, possibly a parse error at `%s'", argv0,
                   TABLE_PATH);
    exit(EXIT_FAILURE);
  }

  if (enable_debug) {  // so we don't iterate for no reason
    for (const auto& row : table_config) {
      daemon_log(LOG_DEBUG, "  %d -> %d", row.x, row.y);
    }
  }

  debug_log("reading pwm_cap file `%s'", PWM_CAP_PATH);
  unsigned pwm_cap = read_file_int(PWM_CAP_PATH);

  /*
   * scan temperature sensors
   */
  debug_log("ignoring sensor containing `%s'", options_object.substring.c_str());
  vector<string> sensor_paths = scan_sensors(options_object.substring.c_str());

  /*
   * daemon loop
   */
  while (true) {
    temperature = thermal_average(sensor_paths, options_object.use_highest);
    temperature /= 1000;

    if (enable_max_freq) {
      // if fantable.service runs AFTER nvpmodel.service this should not be necessary
      if (clocks_wait <= 0) {
        if (clocks_did_set == false) {
          debug_log("maxing out clock frequencies");
          clocks_max_freq();
          clocks_did_set = true;
        }
      } else {
        clocks_wait--;
      }
    }

    if (temperature != temperature_old) {
      pwm = interpolate(table_config, temperature);

      // print PWM speed from table before its changed
      debug_log("temperature: %dC", temperature);
      debug_log("fan speed: %d%%", pwm);

      pwm = pwm * pwm_cap / 100;

      // make sure it's between the bounds
      pwm = std::clamp(pwm, unsigned(0), pwm_cap);

      debug_log("target_pwm: %d", pwm);
      debug_log("writing pwm to `%s'", TARGET_PWM_PATH);
#if WRITE_SYSTEM_FILES_DANGEROUS
      write_file_int(TARGET_PWM_PATH, pwm);
#endif
    }

    temperature_old = temperature;

    std::this_thread::sleep_for(interval);
  }

  return 0;
}
