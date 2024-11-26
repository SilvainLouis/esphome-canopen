#include "ota.h"
// #ifdef USE_OTA
#include "esphome/components/md5/md5.h"
#include "esphome/components/ota/ota_backend.h"
#include "esphome/components/ota/ota_backend_esp_idf.h"
#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/core/util.h"
#include "esphome/core/base_automation.h"

#include <cerrno>
#include <cstdio>

namespace esphome {
namespace canopen {

void CanopenOTAComponent::setup() {
#ifdef USE_OTA_STATE_CALLBACK
  ota::register_ota_platform(this);
#endif
  backend = esphome::ota::make_ota_backend();

  ota_finished_trigger = new canopen::OtaFinishedTrigger();
  auto automation_id = new Automation<>(ota_finished_trigger);
  auto delayaction_id = new DelayAction<>();
  delayaction_id->set_component_source("canopen.ota");
  App.register_component(delayaction_id);
  delayaction_id->set_delay(5000);
  auto lambdaaction_id = new LambdaAction<>([=]() -> void {
    ESP_LOGI(TAG, "ota finished, rebooting");
    App.safe_reboot();
  });
  automation_id->add_actions({delayaction_id, lambdaaction_id});
}

void CanopenOTAComponent::loop() {}

float CanopenOTAComponent::get_setup_priority() const { return setup_priority::LATE; }

esphome::ota::OTAResponseTypes CanopenOTAComponent::begin(uint32_t size) {
  this->stream = z_stream{};
  this->stream.next_out = this->s_outbuf;
  this->stream.avail_out = BUF_SIZE;
  this->stream.avail_in = 0;
  this->written = 0;

  if (mz_inflateInit(&this->stream)) {
    ESP_LOGW(TAG, "cannot initialize decompressor");
  }
  return !dry_run ? backend->begin(size) : esphome::ota::OTAResponseTypes::OTA_RESPONSE_OK;
}

int CanopenOTAComponent::decompress() {
  auto status = inflate(&this->stream, Z_SYNC_FLUSH);
  if ((status == Z_STREAM_END) || (status == Z_OK && !this->stream.avail_out)) {
    uint32_t n = BUF_SIZE - this->stream.avail_out;
    this->written += n;
    ESP_LOGI(TAG, "writing %d bytes to flash, total: %d", n, this->written);
    auto ret = !dry_run ? backend->write(this->s_outbuf, n) : esphome::ota::OTAResponseTypes::OTA_RESPONSE_OK;
    if (ret != esphome::ota::OTAResponseTypes::OTA_RESPONSE_OK) {
      ESP_LOGW("ota", "write flash error: %d", ret);
      return -10;
    }
    this->stream.next_out = this->s_outbuf;
    this->stream.avail_out = BUF_SIZE;
  }
  return status;
}

esphome::ota::OTAResponseTypes CanopenOTAComponent::write(uint8_t *data, size_t len) {
  this->stream.next_in = data;
  this->stream.avail_in = len;

  while (stream.avail_in) {
    int status = this->decompress();
    if (status == Z_STREAM_END) {
      break;
    }
    if (status != Z_OK) {
      ESP_LOGW(TAG, "decompression failed with %d", status);
      return esphome::ota::OTAResponseTypes::OTA_RESPONSE_ERROR_UNKNOWN;
    }
  }
  return esphome::ota::OTAResponseTypes::OTA_RESPONSE_OK;
}

esphome::ota::OTAResponseTypes CanopenOTAComponent::end(const char *expected_md5) {
  esphome::ota::OTAResponseTypes ret;
  for (;;) {
    auto status = this->decompress();
    if (status == Z_STREAM_END) {
      if (!dry_run) {
        backend->set_update_md5(expected_md5);
        ret = backend->end();
      } else {
        ret = esphome::ota::OTAResponseTypes::OTA_RESPONSE_OK;
      }
      if (!ret) {
        ESP_LOGI(TAG, "decompression finished successfully");
        ota_finished_trigger->trigger();
      }
      break;
    } else if (status != Z_OK) {
      ESP_LOGW(TAG, "decompression failed with %d", status);
      ret = esphome::ota::OTAResponseTypes::OTA_RESPONSE_ERROR_UNKNOWN;
      break;
    }
  }
  mz_deflateEnd(&this->stream);
  return ret;
}

}  // namespace canopen
}  // namespace esphome
// #endif
