// core event callbacks
// include this after core.h

static inline void core_proc_window_event(SDL_WindowEvent *ev, core_t *core) {
  switch (ev->event) {
    case SDL_WINDOWEVENT_CLOSE:
      if (ev->windowID == SDL_GetWindowID(core->sdl.v.win))
        core->state.active_flag = 0;
      else
        SDL_HideWindow(SDL_GetWindowFromID(ev->windowID));
      break;

    default: break;
  }
}

#define SETINPUT(i, v) p->cur.btns = BITCHG(p->cur.btns, (i), (v))

static void key_switch(nes_player_input_t *p, int i, int cmd){
  switch (i) {
    case CTRLS_KEY_A: SETINPUT(NES_INPUT_BIT_A, cmd); break;
    case CTRLS_KEY_B: SETINPUT(NES_INPUT_BIT_B, cmd); break;
    case CTRLS_KEY_SELECT: SETINPUT(NES_INPUT_BIT_SELECT, cmd); break;
    case CTRLS_KEY_START: SETINPUT(NES_INPUT_BIT_START, cmd); break;
    case CTRLS_KEY_UP: SETINPUT(NES_INPUT_BIT_UP, cmd); break;
    case CTRLS_KEY_DOWN: SETINPUT(NES_INPUT_BIT_DOWN, cmd); break;
    case CTRLS_KEY_LEFT: SETINPUT(NES_INPUT_BIT_LEFT, cmd); break;
    case CTRLS_KEY_RIGHT: SETINPUT(NES_INPUT_BIT_RIGHT, cmd); break;
  }
}

static void core_proc_event_key(core_controls_t *ctrls, nes_input_t *input, SDL_Keycode key, char action) {
  for (int i = 0; i < CTRLS_KEY_COUNT; ++i)
    if (key == ctrls->p1.code[i]) {
      key_switch(&input->p1, i, !action);
      return;
    }

  for (int i = 0; i < CTRLS_KEY_COUNT; ++i)
    if (key == ctrls->p2.code[i]) {
      key_switch(&input->p2, i, !action);
      return;
    }
}

static void core_proc_event(SDL_Event *ev, void *pcore) {
  core_t *core = pcore;
  switch (ev->type) {
    case SDL_WINDOWEVENT:
      core_proc_window_event(&ev->window, core);
      break;

    case SDL_QUIT: core->state.active_flag = 0; break;

    case SDL_KEYDOWN:
      core_proc_event_key(&core->ctrls, &core->nes.input, ev->key.keysym.sym,
                          0);
      break;

    case SDL_KEYUP:
      core_proc_event_key(&core->ctrls, &core->nes.input, ev->key.keysym.sym,
                          1);
      break;

    default: break;
  }
}
