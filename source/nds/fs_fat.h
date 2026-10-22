// Storage on the flashcart's microSD through DLDI: fat:/data/pocketsheet/.
// This is the only place a drive path is written, so the app can't touch
// sd:/ (the DSi's own slot) or nand:/.
#pragma once

#include "ui/app.h"

void fs_fat_storage(Storage *out);
