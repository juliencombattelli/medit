#ifndef MEDIT_LOADER_H_
#define MEDIT_LOADER_H_

#include "loader_app_main.h"

MeditAppResult medit_loader_reload_app(int argc, char** argv, void* old_app_state);

int medit_loader_main(int argc, char** argv);

#endif // MEDIT_LOADER_H_
