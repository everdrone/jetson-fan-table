#include <assert.h>
#include <getopt.h>
// #include <libgen.h>

#include <algorithm>
#include <chrono>
#include <thread>

#include "atexit.h"
#include "config.h"
#include "defines.h"
#include "interpolate.h"
#include "jetson_clocks.h"
#include "load_config.h"
#include "log.h"
#include "parse_table.h"
#include "pid.h"
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
      // clang-format off
      "%s [options]\n"
      "    -h --help                       Show this help\n"
      "    -v --version                    Show version\n"
      "    -i --interval <int>             Interval in seconds (defaults to 2)\n"
      "    -t --enable-tach                Enable the fan tachometer for speed monitoring\n"
      "    -c --check                      Returns 0 if process is running\n"
      "    -s --status                     Print process status\n"
      "                                    Can be used with options -I and -A\n"
      "    -M --no-max-freq                Do not set CPU and GPU clocks\n"
      "    -A --no-average                 Use the highest measured temperature instead of\n"
      "                                    calculating the average\n"
      "    -I --ignore-sensors <string>    Ignore sensors that match a substring (case sensitive)\n"
      "       --debug                      Increase verbosity in syslog\n",
      // clang-format on
      argv0);
  exit(EXIT_SUCCESS);
}

void print_status(const char* ingore_substr, bool use_highest) {
  pid_t pid;
  int retval = ESRCH;

  if ((pid = pid_file_is_running()) >= 0) {
    printf("process pid: %d\n", pid);

    vector<string> sensor_paths = scan_sensors(ingore_substr);
    unsigned temperature = 0;
    unsigned fan_pwm = 0;
    unsigned cur_rpm = 0;

    temperature = thermal_average(sensor_paths, use_highest);
    fan_pwm = read_file_int(CUR_PWM_PATH);

    printf("temperature: %d C\n", temperature / 1000);
    printf("current pwm: %d\n", fan_pwm);

    if (read_file_int(TACH_ENABLE_PATH) == 1) {
      cur_rpm = read_file_int(MEASURED_RPM_PATH);
      printf("current rpm: %d\n", cur_rpm);
    } else {
      printf("tachometer is disabled\n");
    }

    retval = EXIT_SUCCESS;
  } else {
    sprintf_stderr("%s is not running", argv0);
  }

  exit(retval);
}

void check_pid() {
  pid_t pid;
  if ((pid = pid_file_is_running()) >= 0) {
    exit(EXIT_SUCCESS);
  } else {
    exit(ESRCH);
  }
}

int main(int argc, char* argv[]) {
  //
  // the program name can be set by using the config.h PACKAGE_NAME
  //
  // argv0 = basename(argv[0]);

  pid_t pid;
  options_t oobj;

  // clang-format off
  static const struct option long_options[] = {
    {"help",            no_argument,        NULL, 'h'},
    {"version",         no_argument,        NULL, 'v'},
    {"interval",        required_argument,  NULL, 'i'},
    {"enable-tach",     no_argument,        NULL, 't'},
    {"check",           no_argument,        NULL, 'c'},
    {"status",          no_argument,        NULL, 's'},
    {"no-max-freq",     no_argument,        NULL, 'M'},
    {"no-average",      no_argument,        NULL, 'A'},
    {"ignore-sensors",  required_argument,  NULL, 'I'},
    {"debug",           no_argument,        NULL, OPTION_DEBUG},
    {NULL,              0,                  NULL, 0}};
  // clang-format on

  int opt;
  while ((opt = getopt_long(argc, argv, "hvcsi:tMAI:", long_options, NULL)) >= 0) {
    switch (opt) {
      case 'h':
        oobj.help = true;
        break;
      case 'v':
        oobj.version = true;
        break;
      case 't':
        enable_tach = true;
        break;
      case 'c':
        oobj.check = true;
        break;
      case 's':
        oobj.status = true;
        break;
      case 'M':
        enable_max_freq = false;
        break;
      case 'A':
        oobj.use_highest = true;
        break;
      case 'i':
        try {
          oobj.interval = std::stoul(optarg);
        } catch (...) {
          daemon_log(LOG_ERR, "cannot parse argument `%s' for --interval", optarg);
          fprintf(stderr, "%s: cannot parse argument `%s' for --interval\n", argv0, optarg);
          exit(EXIT_FAILURE);
        }
        break;
      case 'I':
        oobj.substring = optarg;
        break;
      case OPTION_DEBUG:
        enable_debug = true;
        break;
      case '?':
        if (optopt == 'i' || optopt == 'I')
          fprintf(stderr, "Option -%c requires an argument\n", optopt);
        else if (isprint(optopt))
          fprintf(stderr, "Unknown option `-%c'\n", optopt);
        else
          fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
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
  if (oobj.help) {
    print_help_exit();
  }

  // print version and exit
  if (oobj.version) {
    print_version_exit();
  }

  // check pid and exit
  if (oobj.check) {
    check_pid();
  }

  // --help, --version and --check don't need the config
  load_config(&oobj);

  if (oobj.status) {
    // print daemon status + information and exit
    print_status(oobj.substring.c_str(), oobj.use_highest);
  }

#ifdef DEBUG_OPTIONS
  std::cout << "help            " << oobj.help << std::endl;
  std::cout << "version         " << oobj.version << std::endl;
  std::cout << "check           " << oobj.check << std::endl;
  std::cout << "status          " << oobj.status << std::endl;
  std::cout << "enable_debug    " << enable_debug << std::endl;

  std::cout << "interval        " << oobj.interval << std::endl;
  std::cout << "substring       " << oobj.substring << std::endl;
  std::cout << "enable_max_freq " << enable_max_freq << std::endl;
  std::cout << "use_highest     " << oobj.use_highest << std::endl;
  std::cout << "enable_tach     " << enable_tach << std::endl;
#endif

  if (!is_sudo_or_root()) {
    daemon_log(LOG_INFO, "requested operation requires superuser privilege");
    sprintf_stderr("%s: requested operation requires superuser privilege", argv0);
    exit(EACCES);
  }

  // Start logging
  daemon_log(LOG_INFO, "Starting fan control daemon...");

  register_exit_handler();

  /*
   * check and set pid file
   */
  // pid_file_is_running() can set errno = ENOENT
  int saved_errno = errno;
  if ((pid = pid_file_is_running()) > 0) {
    daemon_log(LOG_ERR, "process is already running with pid %d", pid);
    sprintf_stderr("%s: process is already running with pid %d", argv0, pid);
    exit(EXIT_FAILURE);
  } else {
    debug_log("creating pid file");

    if (errno == ENOENT) {
      // ignore ENOENT if everything is ok
      errno = saved_errno;
    }

    if (pid_file_create() < 0) {
      daemon_log(LOG_ERR, "cannot create pid file");
      sprintf_stderr("%s: cannot create pid file", argv0);
      exit(EXIT_FAILURE);
    }
  }

  // check if is first time running
  if (access(INITIAL_STORE_FILE, F_OK) == -1) {
    is_first_run = true;
  }

  // cleanup before jetson_clocks saves again
  remove_file(STORE_FILE);
  daemon_log(LOG_INFO, "saving state to: `%s'", is_first_run ? INITIAL_STORE_FILE : STORE_FILE);
  store_config(is_first_run ? INITIAL_STORE_FILE : STORE_FILE);

  // enbale tachomenter
  if (enable_tach) {
#if WRITE_SYSTEM_FILES_DANGEROUS
    debug_log("enabling tachometer");
    tach_state = read_file_int(TACH_ENABLE_PATH);
    write_file_int(TACH_ENABLE_PATH, 1);
#endif
  }

  debug_log("using interval of %d seconds", oobj.interval);
  std::chrono::seconds interval(oobj.interval);

  unsigned temperature;
  int temperature_old = -1;
  unsigned pwm;

  // we can also skip if the process starts after nvpmodel.service
  unsigned clocks_wait = MAX_FREQ_WAIT / oobj.interval;

  if (oobj.use_highest) {
    debug_log("using highest measured temperature");
  }

  /*
   * read table file
   */
  debug_log("using table file `%s'", TABLE_PATH);

  vector<coord_t> table_config = parse_table(TABLE_PATH, true);

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
  debug_log("ignoring sensor containing `%s'", oobj.substring.c_str());
  vector<string> sensor_paths = scan_sensors(oobj.substring.c_str());

  /*
   * daemon loop
   */
  while (true) {
    temperature = thermal_average(sensor_paths, oobj.use_highest);
    temperature /= 1000;

    if (enable_max_freq) {
      // if fantable runs AFTER nvpmodel.service this should not be necessary
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

#if WRITE_SYSTEM_FILES_DANGEROUS
      debug_log("target_pwm: %d", pwm);
      debug_log("writing pwm to `%s'", TARGET_PWM_PATH);
      write_file_int(TARGET_PWM_PATH, pwm);
#endif
    }

    temperature_old = temperature;

    std::this_thread::sleep_for(interval);
  }

  return 0;
}
