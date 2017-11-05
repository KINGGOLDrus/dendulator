#pragma once

// error codes
enum errcode {
  NO_ERR,       // no error
  ERR_ARGS,     // invalid args
  ERR_SDL_INIT, // SDL init error
  ERR_ROM_LOAD, // ROM loading error
  ERR_ROM_INIT, // ROM initialization error
};
