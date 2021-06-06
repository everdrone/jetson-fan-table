#include <assert.h>
#include <getopt.h>

#include <algorithm>
#include <chrono>
#include <thread>

#include "atexit.h"
#include "clocks.h"
#include "config.h"
#include "defines.h"
#include "interpolate.h"
#include "log.h"
#include "parse_table.h"
#include "thermal.h"
#include "utils.h"

/* TODO: use const char* where possible
 */

void print_version_exit() {
  std::cout << PACKAGE_STRING << std::endl;
  exit(EXIT_SUCCESS);
}

void print_help_exit() {
  printf(
      "%s [options]\n"
      "    -h --help           Show this help\n"
      "    -v --version        Show version\n"
      // "    -c --check          Check table file\n"
      "    -i --interval       Interval in seconds (defaults to 2)\n"
      "                        calculating the average\n"
      "    -M --no-max-freq    Do not set CPU and GPU clocks\n"
      "       --debug          Increase verbosity in syslog\n",
      argv0);
  exit(EXIT_SUCCESS);
}

enum options_enum {
  OPTION_DEBUG = 256,
};

typedef struct {
  bool help = false;
  bool version = false;
  bool check = false;
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
    {"help",        no_argument         NULL, 'h'},
    {"version",     no_argument         NULL, 'v'},
    // {"check",       no_argument         NULL, 'c'},
    {"interval",    required_argument,  NULL, 'i'},
    {"no-max-freq", no_argument,        NULL, 'M'},
    {"debug",       no_argument,        NULL, OPTION_DEBUG},
    {NULL,          0,                  NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "hvi:M", long_options, NULL)) >= 0) {
    switch (opt) {
    case 'h':
      options_object.help = true;
      break;
    case 'v':
      options_object.version = true;
      break;
    // case 'c':
    //   options_object.check = true;
    //   break;
    case 'M':
      enable_max_freq = false;
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

  // Start logging
  daemon_log(LOG_INFO, "Starting fan control daemon...");

  register_exit_handler();

  debug_log("using interval of %d seconds", options_object.interval);

  std::chrono::seconds interval(options_object.interval);

  unsigned temperature;
  int temperature_old = -1;
  unsigned pwm;

  /*
   * read table file
   */
  debug_log("using table file `%s'", TABLE_PATH);

  std::vector<coord_t> table_config = parse_table(TABLE_PATH);

  // for (const auto& row : table_config) {
  //   std::cout << row.x << "->" << row.y << std::endl;
  // }

  if (table_config.size() < 1) {
    // TODO: handle invalid (empty?) table file
    sprintf_stderr("%s: empty table configuration, possibly a parse error at `%s'", argv0,
                   TABLE_PATH);
    exit(EXIT_FAILURE);
  }

  if (enable_debug) { // so we don't iterate for no reason
    for (const auto &row : table_config) {
      daemon_log(LOG_DEBUG, "  %d -> %d", row.x, row.y);
    }
  }

  debug_log("reading pwm_cap file `%s'", PWM_CAP_PATH);
  unsigned pwm_cap = read_file_int(PWM_CAP_PATH);

  if (enable_max_freq) {
    debug_log("running jetson-clocks --store");
    jetson_clocks_store("/");

    debug_log("running jetson-clocks");
    jetson_clocks_enable();
  }

  /*
   * daemon loop
   */
  while (true) {
    // TODO: add option to use max freq
    // TODO: add option to add substring to ignore in sensor name
    temperature = thermal_average(false);

    if (temperature != temperature_old) {
      pwm = interpolate(table_config, temperature / 1000);

      // print PWM speed from table before its changed
      debug_log("temperature: %dC", temperature / 1000);
      debug_log("fan speed: %d%%", pwm);

      pwm = pwm * pwm_cap / 100;

      // make sure it's between the bounds
      pwm = std::clamp(pwm, unsigned(0), pwm_cap);

      debug_log("target_pwm: %d", pwm);
      debug_log("writing pwm to `%s'", TARGET_PWM_PATH);

      write_file_int(TARGET_PWM_PATH, pwm);
    }

    temperature_old = temperature;

    // FIXME: is this daemon-safe?
    std::this_thread::sleep_for(interval);
  }

  return 0;
}
