#pragma once

std::string create_txt_filename(SCStudyInterfaceRef sc);
SCString create_string_for_sierra_log(SCStudyInterfaceRef sc);
void log_into_txt_file(SCStudyInterfaceRef sc);
bool is_rth(SCStudyInterfaceRef sc);