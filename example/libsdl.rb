module LibSDL
  extend Fiddle::Importer

  # SDL Init flags
  SDL_INIT_TIMER                = 0x00000001
  SDL_INIT_AUDIO                = 0x00000010
  SDL_INIT_VIDEO                = 0x00000020
  SDL_INIT_JOYSTICK             = 0x00000200
  SDL_INIT_HAPTIC               = 0x00001000
  SDL_INIT_GAMECONTROLLER       = 0x00002000
  SDL_INIT_EVENTS               = 0x00004000
  SDL_INIT_NOPARACHUTE          = 0x00100000
  SDL_INIT_EVERYTHING           = SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO |
    SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER

  # SDL Window position flags
  SDL_WINDOWPOS_CENTERED_MASK   = 0x2FFF0000
  SDL_WINDOWPOS_CENTERED        = 0x2FFF0000
  SDL_WINDOWPOS_UNDEFINED_MASK  = 0x1FFF0000
  SDL_WINDOWPOS_UNDEFINED       = SDL_WINDOWPOS_UNDEFINED_MASK | 0

  # SDL Window show flags
  SDL_WINDOW_FULLSCREEN         = 0x00000001
  SDL_WINDOW_OPENGL             = 0x00000002
  SDL_WINDOW_SHOWN              = 0x00000004
  SDL_WINDOW_HIDDEN             = 0x00000008
  SDL_WINDOW_BORDERLESS         = 0x00000010
  SDL_WINDOW_RESIZABLE          = 0x00000020
  SDL_WINDOW_MINIMIZED          = 0x00000040
  SDL_WINDOW_MAXIMIZED          = 0x00000080
  SDL_WINDOW_INPUT_GRABBED      = 0x00000100
  SDL_WINDOW_INPUT_FOCUS        = 0x00000200
  SDL_WINDOW_MOUSE_FOCUS        = 0x00000400
  SDL_WINDOW_FULLSCREEN_DESKTOP = SDL_WINDOW_FULLSCREEN | 0x00001000
  SDL_WINDOW_FOREIGN            = 0x00000800

  # SDL Renderer flags
  SDL_RENDERER_SOFTWARE         = 0x00000001
  SDL_RENDERER_ACCELERATED      = 0x00000002
  SDL_RENDERER_PRESENTVSYNC     = 0x00000004
  SDL_RENDERER_TARGETTEXTURE    = 0x00000008

  SDL_QUIT                      = 0x100

  dlload "libSDL2.so"

  typedef "Uint8" , "unsigned char"
  typedef "Uint16", "unsigned short"
  typedef "Uint32", "unsigned int"
  typedef "Uint64", "unsigned long long"
  typedef "Sint8" , "char"
  typedef "Sint16", "short"
  typedef "Sint32", "int"
  typedef "Sint64", "long long"

  SDL_Rect  = struct ["int x", "int y", "int w", "int h"]
  SDL_Event = union ["Uint32 type", "Uint8 padding[56]"]

  extern "Uint32 SDL_Init(Uint32)"
  extern "const char * SDL_GetError()"
  extern "void * SDL_CreateWindow(const char *, int, int, int, int, Uint32)"
  extern "SDL_Renderer * SDL_CreateRenderer(SDL_Window *, int, Uint32)"
  extern "int SDL_SetRenderDrawColor(SDL_Renderer *, Uint8, Uint8, Uint8, Uint8)"
  extern "void SDL_DestroyRenderer(SDL_Renderer *)"
  extern "void SDL_DestroyWindow(SDL_Window *)"
  extern "int SDL_PollEvent(SDL_Event *)"
  extern "int SDL_RenderFillRect(SDL_Renderer *, const SDL_Rect *)"
  extern "int SDL_RenderDrawRect(SDL_Renderer *, const SDL_Rect *)"
  extern "int SDL_RenderDrawLine(SDL_Renderer *, int, int, int, int)"
  extern "int SDL_RenderDrawPoint(SDL_Renderer *, int, int)"
  extern "void SDL_Quit()"
  extern "int SDL_RenderClear(SDL_Renderer *)"
  extern "void SDL_RenderPresent(SDL_Renderer *)"
  extern "void SDL_Delay(Uint32)"

  def error
    SDL_GetError.to_str
  end
end

class Game
  attr_reader :width, :height

  def initialize(width=800, height=600)
    @width = width
    @height = height
    @quit = false
  end

  def init
    if LibSDL.SDL_Init(LibSDL::SDL_INIT_VIDEO) < 0 then
      raise RuntimeError, "Could not initialize SDL! #{LibSDL.error}"
    end

    @window = LibSDL.SDL_CreateWindow("SDL mruby", LibSDL::SDL_WINDOWPOS_CENTERED,
      LibSDL::SDL_WINDOWPOS_CENTERED, @width, @height, LibSDL::SDL_WINDOW_SHOWN)
    if @window == nil then
      raise RuntimeError, "Could not create window! #{LibSDL.error}"
    end

    @renderer = LibSDL.SDL_CreateRenderer(@window, -1, LibSDL::SDL_RENDERER_ACCELERATED)
    if @renderer == nil then
      raise RuntimeError, "Could not create renderer! #{LibSDL.error}"
    end

    LibSDL.SDL_SetRenderDrawColor(@renderer, 0xFF, 0xFF, 0xFF, 0xFF)

    @fill_rect = LibSDL::SDL_Rect.malloc(@width/4, @height/4, @width/2, @height/2)
    @outline_rect = LibSDL::SDL_Rect.malloc(@width/6, @height/6, @width * 2 / 3, @height * 2 / 3)
  end

  def render
    LibSDL.SDL_SetRenderDrawColor(@renderer, 0xFF, 0xFF, 0xFF, 0xFF)
    LibSDL.SDL_RenderClear @renderer

    LibSDL.SDL_SetRenderDrawColor(@renderer, 0xFF, 0x00, 0x00, 0xFF)
    LibSDL.SDL_RenderFillRect(@renderer, @fill_rect)

    LibSDL.SDL_SetRenderDrawColor(@renderer, 0x00, 0xFF, 0x00, 0xFF)
    LibSDL.SDL_RenderDrawRect(@renderer, @outline_rect)

    LibSDL.SDL_SetRenderDrawColor(@renderer, 0x00, 0x00, 0xFF, 0xFF)
    LibSDL.SDL_RenderDrawLine(@renderer, 0, @height/2, @width, @height/2)

    LibSDL.SDL_SetRenderDrawColor(@renderer, 0xFF, 0xFF, 0x00, 0xFF)
    i = 0
    while i < @height do
      LibSDL.SDL_RenderDrawPoint(@renderer, @width/2, i)
      i = i + 4
    end

    LibSDL.SDL_RenderPresent @renderer
    LibSDL.SDL_Delay 1
  end

  def start
    init
    event = LibSDL::SDL_Event.malloc
    while not @quit do
      while LibSDL.SDL_PollEvent(event) != 0 do
        case event.type
        when LibSDL::SDL_QUIT
          @quit = true
        end
      end
      render
    end
    event.destroy
    quit
  end

  def quit
    @fill_rect.destroy
    @outline_rect.destroy

    LibSDL.SDL_DestroyRenderer(@renderer)
    LibSDL.SDL_DestroyWindow(@window)
    LibSDL.SDL_Quit
  end
end

Game.new.start