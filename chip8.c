#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL.h>


typedef struct{
    SDL_Window *window;     //SDL Window object
    SDL_Renderer * renderer //SDL Renderer object

}sdl_t;


typedef struct{
    uint32_t window_height; //SDL Window height
    uint32_t window_width;  //SDL Window width
    uint32_t fg_color;      //Foreground Color of the window
    uint32_t bg_color;      //Background Color of the window
    uint32_t scale_factor   //Amount to scale the pixel 
      
}config_t;





bool init_sdl(sdl_t *sdl,const config_t config){
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) !=0 ){
        SDL_Log("Could not initialize SDL subsystems! %s\n",SDL_GetError());
        return false;
    }


    sdl->window=SDL_CreateWindow("CHIP8 EMULATOR",
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                config.window_width*config.scale_factor,
                                config.window_height*config.scale_factor,0);
    
    if(!sdl->window){
        SDL_Log("Could not create SDL window! %s\n",SDL_GetError());
        return false;
    }

    sdl->renderer=SDL_CreateRenderer(sdl->window,-1,SDL_RENDERER_ACCELERATED);

    if(!sdl->renderer){
        SDL_Log("Could not create SDL renderer! %s\n",SDL_GetError());
    }
    return true;
}


//Set  up inital emulator configuration  from passed  in arguments
bool set_config_from_args(config_t *config,int argc,char** argv){
    //Default configuration
    
    config->window_width=64;       //Chip8 original x resolution
    config->window_height=32;      //Chip8 original y resolution 
    config->fg_color=0xFFFFFFFF;   //White
    config->bg_color=0xFFFF00FF;   //Black
    config->scale_factor=20;       //Default scaling of the window

    //Setting the the passed in arguments
    for(int i=1;i<argc;i++){
        (void)argv[i];

    }

    return true;
}

//Final claenup of all created objects
void final_cleanup(const sdl_t sdl){
    SDL_DestroyRenderer(sdl.renderer);
    SDL_DestroyWindow(sdl.window);
    SDL_Quit();
}

//Screan clear
void clear_screen(const config_t config,const sdl_t sdl){
    const uint8_t r= (config.bg_color >> 24) & 0xFF;
    const uint8_t g= (config.bg_color >> 16) & 0xFF;
    const uint8_t b= (config.bg_color >> 8 ) & 0xFF;
    const uint8_t a= (config.bg_color >> 0 ) & 0xFF;
    SDL_SetRenderDrawColor(sdl.renderer,r,g,b,a);
    SDL_RenderClear(sdl.renderer);
}


//Update window with change 
void update_screen(const sdl_t sdl){
    SDL_RenderPresent(sdl.renderer);
}


int main(int argc,char **argv){
    (void) argc;
    (void) argv;

    //Initializing the configaration
    config_t config={0};
    if(!set_config_from_args(&config,argc,argv)){
        exit(EXIT_FAILURE);
    }


    //Initializing the SDL Library
    sdl_t sdl={0};
    if(!init_sdl(&sdl,config)){
        exit(EXIT_FAILURE);
    }

    //Initial screen clear
    clear_screen(config,sdl);
  

    //Main emulator loop
    while(true){
        //Emulate chip instruction


        //Get_time() elapsed since thet last get_time()


        //Delay for approx 60Hz/60fps
        //SDL_Delay(16-acutal time elapsed to emulate the instruction);
        SDL_Delay(16);

        //Update window with changes

        update_screen(sdl);
    }

    //CLeanup
    final_cleanup(sdl);

    puts("TESTING ON LINUX");
    exit(EXIT_SUCCESS);
}