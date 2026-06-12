/*
 * file_system.c
 *
 *  Created on: Nov 21, 2024
 *      Author: philbush
 */

#include "file_system.h"
#include "app_threadx.h"
#include "ext_rtc_server.h"
#include "fx_stm32_sd_driver.h"
#include "gpio.h"
#include "logger.h"
#include "persistent_ram.h"
#include "sdmmc.h"
#include "stddef.h"
#include "stdio.h"
#include "threadx_support.h"

#define LINE_BUF_SIZE (512U)

// Struct functions
static uSWIFT_return_code_t _file_system_initialize_card(void);
static uSWIFT_return_code_t _file_system_set_date_time(void);
static uSWIFT_return_code_t _file_system_save_log_line(char *line,
                                                       uint32_t len);
static uSWIFT_return_code_t _file_system_save_gnss_velocities(GNSS *gnss);
static uSWIFT_return_code_t _file_system_save_gnss_breadcrumb_track(GNSS *gnss);
static uSWIFT_return_code_t
_file_system_save_temperature_raw(Temperature *temp);
static uSWIFT_return_code_t _file_system_save_ct_raw(CT *ct);
static uSWIFT_return_code_t _file_system_save_light_raw(Light_Sensor *light);
static uSWIFT_return_code_t
_file_system_save_turbidity_raw(Turbidity_Sensor *obs);

// Helper functions
static void __sd_card_on(void);
static void __sd_card_off(void);
static bool __open_sd_card(void);
static bool __close_sd_card(void);
static bool __write_file_header(char *header, file_index_t file_index);
static bool __close_out_file(char *optional_footer, file_index_t file_index);
static bool __update_file_timestamp(file_index_t file_index);

// Internal object pointer
static File_System_SD_Card *file_sys_self;

// Time format (MM/DD/YY HH:MM:SS)
static char *csv_time_format = "%x %X,";

void file_system_init(File_System_SD_Card *file_system,
                      uint32_t *media_sector_cache, FX_MEDIA *sd_card,
                      microSWIFT_configuration *global_config) {
  char time_str[64] = {0};
  time_t sys_time_now = get_system_time();
  struct tm time_now = *gmtime(&sys_time_now);

  file_sys_self = file_system;

  file_sys_self->sd_card = sd_card;
  file_sys_self->media_sector_cache = media_sector_cache;
  file_sys_self->global_config = global_config;

  // This is made up just for the purpose of having something
  file_sys_self->fet_pin.port = SD_CARD_FET_GPIO_Port;
  file_sys_self->fet_pin.pin = SD_CARD_FET_Pin;

  file_sys_self->timer_timeout = false;

  file_sys_self->sample_window_counter =
      persistent_ram_get_sample_window_counter();

  // Get a string of the system time
  (void)strftime(&time_str[0], sizeof(time_str), "%m-%d-%y_%H-%M-%S",
                 &time_now);

  // Fill in the file names
  snprintf(&(file_sys_self->file_names[LOG_FILE][0]), FILE_MAX_NAME_LEN,
           "Logs/Log_%s.txt", &time_str[0]);
  snprintf(&(file_sys_self->file_names[GNSS_VELOCITIES][0]), FILE_MAX_NAME_LEN,
           "GNSS/Velocity_%s.csv", &time_str[0]);
  snprintf(&(file_sys_self->file_names[GNSS_BREADCRUMB_TRACK][0]),
           FILE_MAX_NAME_LEN, "Tracks/Track_%s.kml", &time_str[0]);
  snprintf(&(file_sys_self->file_names[TEMPERATURE_FILE][0]), FILE_MAX_NAME_LEN,
           "Temperature/Temp_%s.csv", &time_str[0]);
  snprintf(&(file_sys_self->file_names[CT_FILE][0]), FILE_MAX_NAME_LEN,
           "CT/CT_%s.csv", &time_str[0]);
  snprintf(&(file_sys_self->file_names[LIGHT_FILE][0]), FILE_MAX_NAME_LEN,
           "Light/Light_%s.csv", &time_str[0]);
  snprintf(&(file_sys_self->file_names[TURBIDITY_FILE][0]), FILE_MAX_NAME_LEN,
           "Turbidity/Turbidity_%s.csv", &time_str[0]);

  file_sys_self->initialize_card = _file_system_initialize_card;
  file_sys_self->set_date_time = _file_system_set_date_time;
  file_sys_self->save_log_line = _file_system_save_log_line;
  file_sys_self->save_gnss_velocities = _file_system_save_gnss_velocities;
  file_sys_self->save_gnss_breadcrumb_track =
      _file_system_save_gnss_breadcrumb_track;
  file_sys_self->save_temperature_raw = _file_system_save_temperature_raw;
  file_sys_self->save_ct_raw = _file_system_save_ct_raw;
  file_sys_self->save_light_raw = _file_system_save_light_raw;
  file_sys_self->save_turbidity_raw = _file_system_save_turbidity_raw;
}

void file_system_deinit(void) {
  __close_sd_card();
  sdmmc1_deinit();
}

void file_system_timer_expired(ULONG expiration_input) {
  file_sys_self->timer_timeout = true;
}

bool file_system_get_timeout_status(void) {
  return file_sys_self->timer_timeout;
}

static uSWIFT_return_code_t _file_system_initialize_card(void) {
  UINT fx_ret;
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;

  __sd_card_on();

  // Initialize the SDMMC interface
  if (sdmmc1_init() != SDMMC_OK) {
    ret = uSWIFT_IO_ERROR;
    goto done;
  }

  if (!__open_sd_card()) {
    ret = uSWIFT_IO_ERROR;
    goto done;
  }

  // Create the directories
  fx_ret = fx_directory_create(file_sys_self->sd_card, "Logs");
  if ((fx_ret != FX_SUCCESS) && (fx_ret != FX_ALREADY_CREATED)) {
    ret = uSWIFT_IO_ERROR;
    goto done;
  }

  fx_ret = fx_directory_create(file_sys_self->sd_card, "GNSS");
  if ((fx_ret != FX_SUCCESS) && (fx_ret != FX_ALREADY_CREATED)) {
    ret = uSWIFT_IO_ERROR;
    goto done;
  }

  fx_ret = fx_directory_create(file_sys_self->sd_card, "Tracks");
  if ((fx_ret != FX_SUCCESS) && (fx_ret != FX_ALREADY_CREATED)) {
    ret = uSWIFT_IO_ERROR;
    goto done;
  }

  if (file_sys_self->global_config->temperature_enabled) {
    fx_ret = fx_directory_create(file_sys_self->sd_card, "Temperature");
    if ((fx_ret != FX_SUCCESS) && (fx_ret != FX_ALREADY_CREATED)) {
      ret = uSWIFT_IO_ERROR;
      goto done;
    }
  }

  if (file_sys_self->global_config->ct_enabled) {
    fx_ret = fx_directory_create(file_sys_self->sd_card, "CT");
    if ((fx_ret != FX_SUCCESS) && (fx_ret != FX_ALREADY_CREATED)) {
      ret = uSWIFT_IO_ERROR;
      goto done;
    }
  }

  if (file_sys_self->global_config->light_enabled) {
    fx_ret = fx_directory_create(file_sys_self->sd_card, "Light");
    if ((fx_ret != FX_SUCCESS) && (fx_ret != FX_ALREADY_CREATED)) {
      ret = uSWIFT_IO_ERROR;
      goto done;
    }
  }

  if (file_sys_self->global_config->turbidity_enabled) {
    fx_ret = fx_directory_create(file_sys_self->sd_card, "Turbidity");
    if ((fx_ret != FX_SUCCESS) && (fx_ret != FX_ALREADY_CREATED)) {
      ret = uSWIFT_IO_ERROR;
      goto done;
    }
  }

done:
  //  __close_sd_card ();

  return ret;
}

static uSWIFT_return_code_t _file_system_set_date_time(void) {
  time_t sys_time_now = get_system_time();
  struct tm time_struct = *gmtime(&sys_time_now);
  UINT fx_ret;

  fx_ret = fx_system_date_set(time_struct.tm_year + 1900,
                              time_struct.tm_mon + 1, time_struct.tm_mday);
  fx_ret |= fx_system_time_set(time_struct.tm_hour, time_struct.tm_min,
                               time_struct.tm_sec);

  return (fx_ret == FX_SUCCESS) ? uSWIFT_SUCCESS : uSWIFT_FILE_SYSTEM_ERROR;
}

static uSWIFT_return_code_t _file_system_save_log_line(char *line,
                                                       uint32_t len) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  UINT fx_ret;
  static bool first_time = true, file_creation_failed = false;
  bool file_existed = false;
  time_t sys_time_now = get_system_time();

  //  if ( !__open_sd_card () )
  //  {
  //    ret = uSWIFT_IO_ERROR;
  //    goto done;
  //  }

  if (first_time) {
    first_time = false;

    if (sys_time_now < TIMESTAMP_2025_JAN_1) {
      snprintf(&(file_sys_self->file_names[LOG_FILE][0]), FILE_MAX_NAME_LEN,
               "Logs/Log_%s.txt", "BOOT");
    }

    // Create the file
    fx_ret = fx_file_create(file_sys_self->sd_card,
                            &(file_sys_self->file_names[LOG_FILE][0]));
    if ((fx_ret != FX_SUCCESS) && (fx_ret != FX_ALREADY_CREATED)) {
      file_creation_failed = true;
      ret = uSWIFT_IO_ERROR;
      goto done;
    }
  }

  if (file_creation_failed) {
    ret = uSWIFT_IO_ERROR;
    goto done;
  }

  fx_ret = fx_file_open(file_sys_self->sd_card, &file_sys_self->files[LOG_FILE],
                        &(file_sys_self->file_names[LOG_FILE][0]),
                        FX_OPEN_FOR_WRITE);
  if (fx_ret != FX_SUCCESS) {
    ret = uSWIFT_IO_ERROR;
    goto done;
  }

  fx_ret =
      fx_file_relative_seek(&file_sys_self->files[LOG_FILE], 0, FX_SEEK_END);
  if (fx_ret != FX_SUCCESS) {
    ret = uSWIFT_IO_ERROR;
    goto done;
  }

  fx_ret = fx_file_write(&file_sys_self->files[LOG_FILE], line, len);
  if (fx_ret != FX_SUCCESS) {
    ret = uSWIFT_IO_ERROR;
    goto done;
  }

  if (!__close_out_file(NULL, LOG_FILE)) {
    ret = uSWIFT_IO_ERROR;
    goto done;
  }

done:
  //  if ( !__close_sd_card () )
  //  {
  //    ret = uSWIFT_IO_ERROR;
  //  }

  return ret;
}

// Constant strings used in _file_system_save_gnss_velocities
static char *gnss_velocities_csv_header = "Time,V North,V East,V Down\n";
static char *gnss_velocities_format = "%.3f,%.3f,%.3f\n";
/**************************************************************************/
static uSWIFT_return_code_t _file_system_save_gnss_velocities(GNSS *gnss) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  UINT fx_ret;
  char line[LINE_BUF_SIZE];
  time_t timestamp = gnss->sample_window_start_time;
  struct tm time = *gmtime(&timestamp);
  size_t str_index = 0, size_req = 0;
  uint32_t velocity_index = 0, time_counter = 0;

  //  if ( !__open_sd_card () )
  //  {
  //    ret = uSWIFT_IO_ERROR;
  //    goto done;
  //  }

  if (!__write_file_header(gnss_velocities_csv_header, GNSS_VELOCITIES)) {
    ret = uSWIFT_IO_ERROR;
    goto done;
  }

  // Write the CSV file lines
  while (velocity_index < gnss->total_samples) {
    // Update the timestamp second when appropriate
    if (((time_counter % file_sys_self->global_config->gnss_sampling_rate) ==
         0) &&
        (velocity_index != 0)) {
      time = *gmtime(&timestamp);
      timestamp++;
    }

    // Write the timstamp string
    str_index = strftime(&(line[0]), LINE_BUF_SIZE, csv_time_format, &time);

    // Write the velocities to the char buffer
    size_req = snprintf(
        &(line[str_index]), LINE_BUF_SIZE - str_index, gnss_velocities_format,
        gnss->GNSS_N_Array[velocity_index], gnss->GNSS_E_Array[velocity_index],
        gnss->GNSS_D_Array[velocity_index]);
    // Make sure we got everything
    if (size_req > (LINE_BUF_SIZE - str_index)) {
      ret = uSWIFT_MEMORY_BUFFER_ERROR;
      goto done;
    }
    // Write it!
    fx_ret = fx_file_write(&file_sys_self->files[GNSS_VELOCITIES], &(line[0]),
                           strlen(line));
    if (fx_ret != FX_SUCCESS) {
      ret = uSWIFT_IO_ERROR;
      goto done;
    }

    velocity_index++;
    time_counter++;
  }

  if (!__close_out_file(NULL, GNSS_VELOCITIES)) {
    ret = uSWIFT_IO_ERROR;
  }

done:
  //  if ( !__close_sd_card () )
  //  {
  //    ret = uSWIFT_IO_ERROR;
  //  }
  return ret;
}

// Constant strings used in _file_system_save_gnss_breadcrumb_track
// @formatter:off
static char *gnss_track_kml_header =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n"
    "\t<Document>\n"
    "\t\t<name>microSWIFT %lu track %lu</name>\n"
    "\t\t<Style id=\"circ\">\n"
    "\t\t\t<IconStyle>\n"
    "\t\t\t\t<Icon>\n"
    "\t\t\t\t\t<href>http://maps.google.com/mapfiles/kml/shapes/"
    "placemark_circle.png</href>\n"
    "\t\t\t\t</Icon>\n"
    "\t\t\t</IconStyle>\n"
    "\t\t</Style>\n"
    "\t\t<Style id=\"check-hide-children\">\n"
    "\t\t\t<ListStyle>\n"
    "\t\t\t\t<listItemType>checkHideChildren</listItemType>\n"
    "\t\t\t</ListStyle>\n"
    "\t\t</Style>\n"
    "\t\t<styleUrl>#check-hide-children</styleUrl>\n";
static char *gnss_track_placemark_time_format =
    "\t\t<Placemark>\n"
    "\t\t\t<TimeStamp>\n"
    "\t\t\t\t<when>%Y-%m-%dT%XZ</when>\n"
    "\t\t\t</TimeStamp>\n";
static char *gnss_track_placemark_start_stop_format = "\t\t\t<name>%s</name>\n";
static char *gnss_track_placemark_location_format =
    "\t\t\t<styleUrl>#circ</styleUrl>\n"
    "\t\t\t<Point>\n"
    "\t\t\t\t<coordinates>%.8f,%.8f,0</coordinates>\n"
    "\t\t\t</Point>\n"
    "\t\t</Placemark>\n";
static char *gnss_track_header =
    "\t\t<Placemark>\n"
    "\t\t\t<styleUrl>#yellowLineGreenPoly</styleUrl>\n"
    "\t\t\t<LineString>\n"
    "\t\t\t\t<tessellate>1</tessellate>\n"
    "\t\t\t\t<coordinates>\n";
static char *gnss_track_coord_format = "\t\t\t\t\t%.8f,%.8f,0\n";
static char *gnss_track_kml_footer = "\t\t\t\t</coordinates>\n"
                                     "\t\t\t</LineString>\n"
                                     "\t\t</Placemark>\n"
                                     "\t</Document>\n"
                                     "</kml>";
// @formatter:on
/********************************************************************************/
static uSWIFT_return_code_t
_file_system_save_gnss_breadcrumb_track(GNSS *gnss) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  UINT fx_ret;
  char line[LINE_BUF_SIZE];
  uint32_t str_index = 0;
  uint32_t size_req = 0;
  uint32_t size_remaining = 0;
  uint32_t track_index = 0;
  uint32_t max_points =
      gnss->total_samples / file_sys_self->global_config->gnss_sampling_rate;
  struct tm time;
  time_t timestamp = gnss->sample_window_start_time;

  //  if ( !__open_sd_card () )
  //  {
  //    ret = uSWIFT_IO_ERROR;
  //    goto done;
  //  }

  // Write the tracking number into the header
  snprintf(&(line[0]), LINE_BUF_SIZE, gnss_track_kml_header,
           file_sys_self->global_config->tracking_number,
           file_sys_self->sample_window_counter);

  if (!__write_file_header(&(line[0]), GNSS_BREADCRUMB_TRACK)) {
    ret = uSWIFT_IO_ERROR;
    goto done;
  }

  // Write the timestamped track points
  while (track_index < max_points) {
    time = *gmtime(&timestamp);
    // Write the timstamp string
    str_index = strftime(&(line[0]), LINE_BUF_SIZE,
                         gnss_track_placemark_time_format, &time);
    size_remaining = LINE_BUF_SIZE - str_index;

    if (track_index == 0) {
      str_index += snprintf(&(line[str_index]), size_remaining,
                            gnss_track_placemark_start_stop_format, "Start");
      size_remaining = LINE_BUF_SIZE - str_index;
    }

    if (track_index == (max_points - 1)) {
      str_index += snprintf(&(line[str_index]), size_remaining,
                            gnss_track_placemark_start_stop_format, "Stop");
      size_remaining = LINE_BUF_SIZE - str_index;
    }

    // Write the track point
    size_req = snprintf(&(line[str_index]), size_remaining,
                        gnss_track_placemark_location_format,
                        gnss->breadcrumb_track[track_index].lon,
                        gnss->breadcrumb_track[track_index].lat);
    // Make sure we got everything
    if (size_req > size_remaining) {
      ret = uSWIFT_MEMORY_BUFFER_ERROR;
      goto done;
    }
    // Write it!
    fx_ret = fx_file_write(&file_sys_self->files[GNSS_BREADCRUMB_TRACK],
                           &(line[0]), strlen(line));
    if (fx_ret != FX_SUCCESS) {
      ret = uSWIFT_IO_ERROR;
      goto done;
    }

    track_index++;
    timestamp++;
  }

  // Write the track header
  fx_ret = fx_file_write(&file_sys_self->files[GNSS_BREADCRUMB_TRACK],
                         gnss_track_header, strlen(gnss_track_header));
  if (fx_ret != FX_SUCCESS) {
    ret = uSWIFT_IO_ERROR;
    goto done;
  }

  // Write the track path
  track_index = 0;
  while (track_index < max_points) {
    // Write the track point to the char buffer
    size_req = snprintf(&(line[0]), LINE_BUF_SIZE, gnss_track_coord_format,
                        gnss->breadcrumb_track[track_index].lon,
                        gnss->breadcrumb_track[track_index].lat);
    // Make sure we got everything
    if (size_req > LINE_BUF_SIZE) {
      ret = uSWIFT_MEMORY_BUFFER_ERROR;
      goto done;
    }
    // Write it!
    fx_ret = fx_file_write(&file_sys_self->files[GNSS_BREADCRUMB_TRACK],
                           &(line[0]), strlen(line));

    track_index++;
  }

  if (!__close_out_file(gnss_track_kml_footer, GNSS_BREADCRUMB_TRACK)) {
    ret = uSWIFT_IO_ERROR;
  }

done:
  //  if ( !__close_sd_card () )
  //  {
  //    ret = uSWIFT_IO_ERROR;
  //  }
  return ret;
}

// Constant strings used in _file_system_save_temperature_raw
static char *temperature_csv_header = "Time,temperature\n";
static char *temperature_line_format = "%.3f\n";
static uSWIFT_return_code_t
_file_system_save_temperature_raw(Temperature *temp) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  UINT fx_ret;
  char line[LINE_BUF_SIZE];
  uint32_t str_index = 0, size_req = 0, size_remaining = 0, sample_index = 0,
           max_samples = temp->samples_counter;
  struct tm time;
  time_t timestamp = temp->start_timestamp;

  //  if ( !__open_sd_card () )
  //  {
  //    ret = uSWIFT_IO_ERROR;
  //    goto done;
  //  }

  if (!__write_file_header(temperature_csv_header, TEMPERATURE_FILE)) {
    ret = uSWIFT_IO_ERROR;
    goto done;
  }

  time = *gmtime(&timestamp);
  // Write the timstamp string
  str_index = strftime(&(line[0]), LINE_BUF_SIZE, csv_time_format, &time);
  size_remaining = LINE_BUF_SIZE - str_index;

  // Write the CSV file lines
  while (sample_index < max_samples) {
    // Write the sample values
    size_req = snprintf(&(line[str_index]), size_remaining,
                        temperature_line_format, temp->samples[sample_index]);

    // Make sure we got everything
    if (size_req > size_remaining) {
      ret = uSWIFT_MEMORY_BUFFER_ERROR;
      goto done;
    }

    // Write it!
    fx_ret = fx_file_write(&file_sys_self->files[TEMPERATURE_FILE], &(line[0]),
                           strlen(line));
    if (fx_ret != FX_SUCCESS) {
      ret = uSWIFT_IO_ERROR;
      goto done;
    }

    sample_index++;
  }

  if (!__close_out_file(NULL, TEMPERATURE_FILE)) {
    ret = uSWIFT_IO_ERROR;
  }

done:
  //  if ( !__close_sd_card () )
  //  {
  //    ret = uSWIFT_IO_ERROR;
  //  }
  return ret;
}

// Constant strings used in _file_system_save_turbidity_raw
static char *ct_csv_header = "Time,salinity,temperature\n";
static char *ct_line_format = "%.6f,%.6f\n";
static uSWIFT_return_code_t _file_system_save_ct_raw(CT *ct) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  UINT fx_ret;
  char line[LINE_BUF_SIZE];
  uint32_t str_index = 0, size_req = 0, size_remaining = 0, sample_index = 0,
           max_samples = ct->total_samples;
  struct tm time;
  time_t timestamp = ct->start_timestamp;

  //  if ( !__open_sd_card () )
  //  {
  //    ret = uSWIFT_IO_ERROR;
  //    goto done;
  //  }

  if (!__write_file_header(ct_csv_header, CT_FILE)) {
    ret = uSWIFT_IO_ERROR;
    goto done;
  }

  // Write the CSV file lines
  while (sample_index < max_samples) {
    time = *gmtime(&timestamp);
    // Write the timstamp string
    str_index = strftime(&(line[0]), LINE_BUF_SIZE, csv_time_format, &time);
    size_remaining = LINE_BUF_SIZE - str_index;
    // Write the sample values
    size_req = snprintf(&(line[str_index]), size_remaining, ct_line_format,
                        ct->samples[sample_index].salinity,
                        ct->samples[sample_index].temp);
    // Make sure we got everything
    if (size_req > size_remaining) {
      ret = uSWIFT_MEMORY_BUFFER_ERROR;
      goto done;
    }
    // Write it!
    fx_ret =
        fx_file_write(&file_sys_self->files[CT_FILE], &(line[0]), strlen(line));
    if (fx_ret != FX_SUCCESS) {
      ret = uSWIFT_IO_ERROR;
      goto done;
    }

    sample_index++;
    timestamp += 2; // 2 seconds between measurements with CT sensor
  }

  if (!__close_out_file(NULL, CT_FILE)) {
    ret = uSWIFT_IO_ERROR;
  }

done:
  //  if ( !__close_sd_card () )
  //  {
  //    ret = uSWIFT_IO_ERROR;
  //  }
  return ret;
}

// Constant strings used in _file_system_save_light_raw
static char *light_csv_header = "Time,F1,F2,F3,F4,F5,F6,F7,F8,NIR,Clear,Dark\n";
static char *light_channels_format =
    "%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n";
/*****************************************************************************/
static uSWIFT_return_code_t _file_system_save_light_raw(Light_Sensor *light) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  UINT fx_ret;
  char line[LINE_BUF_SIZE];
  uint32_t size_req = 0;
  uint32_t size_remaining = 0;
  uint32_t str_index = 0;
  uint32_t sample_index = 0;
  uint32_t max_samples = light->valid_samples;
  struct tm time;
  time_t timestamp = light->start_timestamp;

  //  if ( !__open_sd_card () )
  //  {
  //    ret = uSWIFT_IO_ERROR;
  //    goto done;
  //  }

  if (!__write_file_header(light_csv_header, LIGHT_FILE)) {
    ret = uSWIFT_IO_ERROR;
    goto done;
  }

  // Write the CSV file lines
  while (sample_index < max_samples) {
    time = *gmtime(&timestamp);
    // Write the timstamp string
    str_index = strftime(&(line[0]), LINE_BUF_SIZE, csv_time_format, &time);
    size_remaining = LINE_BUF_SIZE - str_index;

    // Write the sample to the char buffer
    size_req =
        snprintf(&(line[str_index]), size_remaining, light_channels_format,
                 light->samples_series[sample_index].f1_chan,
                 light->samples_series[sample_index].f2_chan,
                 light->samples_series[sample_index].f3_chan,
                 light->samples_series[sample_index].f4_chan,
                 light->samples_series[sample_index].f5_chan,
                 light->samples_series[sample_index].f6_chan,
                 light->samples_series[sample_index].f7_chan,
                 light->samples_series[sample_index].f8_chan,
                 light->samples_series[sample_index].nir_chan,
                 light->samples_series[sample_index].clear_chan,
                 light->samples_series[sample_index].dark_chan);
    // Make sure we got everything
    if (size_req > size_remaining) {
      ret = uSWIFT_MEMORY_BUFFER_ERROR;
      goto done;
    }
    // Write it!
    fx_ret = fx_file_write(&file_sys_self->files[LIGHT_FILE], &(line[0]),
                           strlen(line));
    if (fx_ret != FX_SUCCESS) {
      ret = uSWIFT_IO_ERROR;
      goto done;
    }

    sample_index++;
    timestamp += 2;
  }

  if (!__close_out_file(NULL, LIGHT_FILE)) {
    ret = uSWIFT_IO_ERROR;
  }

done:
  //  if ( !__close_sd_card () )
  //  {
  //    ret = uSWIFT_IO_ERROR;
  //  }
  return ret;
}

// Constant strings used in _file_system_save_turbidity_raw
static char *turbidity_csv_header = "Time,proximity,ambient\n";
static char *turbidity_line_format = "%hu,%hu\n";
/*****************************************************************************/
static uSWIFT_return_code_t
_file_system_save_turbidity_raw(Turbidity_Sensor *obs) {
  uSWIFT_return_code_t ret = uSWIFT_SUCCESS;
  UINT fx_ret;
  char line[LINE_BUF_SIZE];
  uint32_t str_index = 0, size_req = 0, size_remaining = 0, sample_index = 0,
           max_samples = obs->samples_counter;
  struct tm time;
  time_t timestamp = obs->start_timestamp;

  //  if ( !__open_sd_card () )
  //  {
  //    ret = uSWIFT_IO_ERROR;
  //    goto done;
  //  }

  if (!__write_file_header(turbidity_csv_header, TURBIDITY_FILE)) {
    ret = uSWIFT_IO_ERROR;
    goto done;
  }

  // Write the CSV file lines
  while (sample_index < max_samples) {
    time = *gmtime(&timestamp);
    // Write the timstamp string
    str_index = strftime(&(line[0]), LINE_BUF_SIZE, csv_time_format, &time);
    size_remaining = LINE_BUF_SIZE - str_index;

    // Write the sample values
    size_req = snprintf(
        &(line[str_index]), size_remaining, turbidity_line_format,
        obs->proximity_series[sample_index], obs->ambient_series[sample_index]);

    // Make sure we got everything
    if (size_req > size_remaining) {
      ret = uSWIFT_MEMORY_BUFFER_ERROR;
      goto done;
    }

    // Write it!
    fx_ret = fx_file_write(&file_sys_self->files[TURBIDITY_FILE], &(line[0]),
                           strlen(line));
    if (fx_ret != FX_SUCCESS) {
      ret = uSWIFT_IO_ERROR;
      goto done;
    }

    sample_index++;
    timestamp++;
  }

  if (!__close_out_file(NULL, TURBIDITY_FILE)) {
    ret = uSWIFT_IO_ERROR;
  }

done:
  //  if ( !__close_sd_card () )
  //  {
  //    ret = uSWIFT_IO_ERROR;
  //  }
  return ret;
}

static void __sd_card_on(void) {
  gpio_write_pin(file_sys_self->fet_pin, GPIO_PIN_SET);
}

static void __sd_card_off(void) {
  gpio_write_pin(file_sys_self->fet_pin, GPIO_PIN_RESET);
}

static bool __open_sd_card(void) {
  UINT fx_ret;
  char card_name[32] = {0};

  //  __sd_card_on ();

  snprintf(&(card_name[0]), 32, "microSWIFT %lu",
           file_sys_self->global_config->tracking_number);

  //  tx_thread_sleep (SOFT_START_DELAY);

  fx_ret =
      fx_media_open(file_sys_self->sd_card, &(card_name[0]), fx_stm32_sd_driver,
                    (VOID *)FX_NULL, (VOID *)file_sys_self->media_sector_cache,
                    FX_STM32_SD_DEFAULT_SECTOR_SIZE);

  return (fx_ret == FX_SUCCESS);
}

static bool __close_sd_card(void) {
  UINT fx_ret;

  fx_ret = fx_media_close(file_sys_self->sd_card);

  //  __sd_card_off ();

  return (fx_ret == FX_SUCCESS);
}

static bool __write_file_header(char *header, file_index_t file_index) {
  UINT fx_ret;

  fx_ret = fx_file_create(file_sys_self->sd_card,
                          &(file_sys_self->file_names[file_index][0]));
  if (fx_ret != FX_SUCCESS) {
    return false;
  }

  fx_ret = fx_file_open(
      file_sys_self->sd_card, &file_sys_self->files[file_index],
      &(file_sys_self->file_names[file_index][0]), FX_OPEN_FOR_WRITE);
  if (fx_ret != FX_SUCCESS) {
    return false;
  }

  fx_ret = fx_file_seek(&file_sys_self->files[file_index], 0);
  if (fx_ret != FX_SUCCESS) {
    return false;
  }

  fx_ret =
      fx_file_write(&file_sys_self->files[file_index], header, strlen(header));

  return (fx_ret == FX_SUCCESS);
}

static bool __close_out_file(char *optional_footer, file_index_t file_index) {
  UINT fx_ret;

  if (optional_footer != NULL) {
    fx_ret = fx_file_write(&file_sys_self->files[file_index], optional_footer,
                           strlen(optional_footer));
    if (fx_ret != FX_SUCCESS) {
      return false;
    }
  }

  if (!__update_file_timestamp(file_index)) {
    return false;
  }

  fx_ret = fx_file_close(&file_sys_self->files[file_index]);
  if (fx_ret != FX_SUCCESS) {
    return false;
  }

  fx_ret = fx_media_flush(file_sys_self->sd_card);

  return (fx_ret == FX_SUCCESS);
}

static bool __update_file_timestamp(file_index_t file_index) {
  UINT fx_ret;
  time_t timestamp_now = get_system_time();
  struct tm time_now = *gmtime(&timestamp_now);

  if (time_now.tm_year < 100) {
    time_now.tm_year = 100;
  }

  fx_ret = fx_file_date_time_set(
      file_sys_self->sd_card, &(file_sys_self->file_names[file_index][0]),
      (time_now.tm_year + 1900), (time_now.tm_mon + 1), time_now.tm_mday,
      time_now.tm_hour, time_now.tm_min, time_now.tm_sec);

  return (fx_ret == FX_SUCCESS);
}
