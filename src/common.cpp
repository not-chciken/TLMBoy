/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko
********************************************************************************/

#include "common.h"

GbCommand::GbCommand() {
}

tlm::tlm_extension_base* GbCommand::clone() const {
  return new GbCommand();
}

void GbCommand::copy_from(tlm::tlm_extension_base const& ext) {
  this->cmd = static_cast<GbCommand const&>(ext).cmd;
}
