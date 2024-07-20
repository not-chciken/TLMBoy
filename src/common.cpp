#include "common.h"

GbCommand::GbCommand() {
}

tlm::tlm_extension_base* GbCommand::clone() const {
  return new GbCommand();
}

void GbCommand::copy_from(tlm::tlm_extension_base const& ext) {
  this->cmd = static_cast<GbCommand const&>(ext).cmd;
}
