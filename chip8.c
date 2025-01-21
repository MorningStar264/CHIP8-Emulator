#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL.h>

//SDL container object
typedef struct{
    SDL_Window *window;     //SDL Window object
    SDL_Renderer * renderer //SDL Renderer object

}sdl_t;

//Chip8 instruction set

typedef struct{
    uint16_t opcode;
    uint16_t NNN;           //12 bit address
    uint8_t NN;             //8  bit cosntant
    uint8_t X;              //4  bit constant
    uint8_t Y;              //4  bit constant
}instruction_t;

//Emulator State
typedef struct{
    uint32_t window_height; //SDL Window height
    uint32_t window_width;  //SDL Window width
    uint32_t fg_color;      //Foreground Color of the window
    uint32_t bg_color;      //Background Color of the window
    uint32_t scale_factor   //Amount to scale the pixel 
      
}config_t;

//Emulator states
typedef enum{
    QUIT,
    RUNNING,
    PAUSED,
}emulator_state_t;

//Chip8 Machine object
typedef struct{
    emulator_state_t state;
    uint8_t ram[4096];
    bool display[64*32];      //Original Chip8 display
    uint16_t stack[12];       //Stack of the Chip8 machine
    uint16_t *stack_ptr;      // Pointer to the stack
    uint8_t V[16];            //Data registers
    uint16_t I;               //Index registers
    uint16_t PC;              //Program counter
    uint8_t delay_timer;      //Decrement at 60Hz and used for game purpouses
    uint8_t sound_timer;      //Decrement at 60Hz and play the tune when >0
    bool keypad[16];          //Hexadecimal keypad 0-F
    const char *rom_name;     //Rom name
    instruction_t inst;       //Currently running instruction
}chip8_t;

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
    config->fg_color=0x00000000;   //White
    config->bg_color=0xFFFFFFFF;   //Black
    config->scale_factor=20;       //Default scaling of the window

    //Setting the the passed in arguments
    for(int i=1;i<argc;i++){
        (void)argv[i];

    }

    return true;
}

//Initiaze CHIP8 machine
bool init_chip8(chip8_t *chip8,const char rom_name[]){
    const int32_t entry_point=0x200;    //CHIP8 roms are loaded at 0x200
    const int8_t font[]={
        0xF0, 0x90, 0x90, 0x90, 0xF0,           //0--------->{11110000}--> this is how the display work
        0x20, 0x60, 0x20, 0x20, 0x70,           //1          {10010000}    
        0xF0, 0x10, 0xF0, 0x80, 0xF0,           //2          {10010000}
        0xF0, 0x10, 0xF0, 0x10, 0xF0,           //3          {10010000}
        0xA0, 0xA0, 0xF0, 0x20, 0x20,           //4          {11110000}
        0xF0, 0x80, 0xF0, 0x10, 0xF0,           //5
        0xF0, 0x80, 0xF0, 0x90, 0xF0,           //6
        0xF0, 0x10, 0x20, 0x40, 0x40,           //7
        0xF0, 0x90, 0xF0, 0x90, 0xF0,           //8
        0xF0, 0x90, 0xF0, 0x10, 0xF0,           //9
        0xF0, 0x90, 0xF0, 0x90, 0x90,           //A
        0xE0, 0x90, 0xE0, 0x90, 0xE0,           //B
        0xF0, 0x80, 0x80, 0x80, 0xF0,           //C
        0xE0, 0x90, 0x90, 0x90, 0xE0,           //D
        0xF0, 0x80, 0xF0, 0x80, 0xF0,           //E
        0xF0, 0x80, 0xF0, 0x80, 0x80,           //F
    };

    //Load font 
    memcpy(&chip8->ram[0],font,sizeof(font));
    
    //Load rom
    FILE *rom= fopen(rom_name,"rb");
    if(!rom){
        SDL_Log("Rom file %s is invalid or does not exist",rom_name);
        return false;
    }

    //Checking rom file and size
    fseek(rom,0,SEEK_END);
    const size_t rom_size=ftell(rom);
    const size_t max_size=sizeof chip8->ram - entry_point;
    rewind(rom);

    if(rom_size > max_size){
        SDL_Log("Rom file %s is beyond chip8 size limit",rom_name);
        return false;
    }

    if(fread(&chip8->ram[entry_point],rom_size,1,rom)!=1){
        SDL_Log("Could not read the rom %s",rom_name);
        return false;
    }


    
    fclose(rom);
    //Set chip8 machine
    chip8->state=RUNNING;                //Default state is running
    chip8->PC=entry_point;               //Setting the program counter to the start of the rom
    chip8->rom_name=rom_name;            //Setting the rom name
    chip8->stack_ptr=&chip8->stack[0];
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

//Handling user input 
void handle_input(chip8_t *chip8){
    SDL_Event event;

    while(SDL_PollEvent(&event)){
        switch(event.type){

            case SDL_QUIT:
                chip8->state=QUIT;
                return;
            
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym){
                    case SDLK_ESCAPE:
                        chip8->state=QUIT;
                        return;
                    
                    case SDLK_SPACE:
                        if(chip8->state==RUNNING){
                            chip8->state=PAUSED;
                        }
                        else{
                            chip8->state=RUNNING;
                            puts("=======PAUSED=======");
                        }
                        return;
                    
                    default:
                        break;
                }
                break;
            
            case SDL_KEYUP:
                break;

            default:
                break;
        }
    }
}
#ifdef DEBUG
    void print_debug_info(chip8_t *chip8){
    }
#endif
void emulate_instruction(chip8_t *chip8){
    //Get the next opcode from memory  
    chip8->inst.opcode=(chip8->ram[chip8->PC]<<8) | chip8->ram[chip8->PC+1];
    chip8->PC+=2;

    //Fill out instruction format
    chip8->inst.NNN=  chip8->inst.opcode  &0x0FFF;
    chip8->inst.NN =  chip8->inst.opcode  &0x00FF;
    chip8->inst.N  =  chip8->inst.opcode  &0x000F;
    chip8->inst.X  =  (chip8->inst.opcode >> 8) &0x000F;
    chip8->inst.Y  =  (chip8->inst.opcode >> 4) &0x000F;

#ifdef DEBUG
    print_debug_info(chip8);
#endif
    switch(chip8->inst.opcode>>12 & 0x000F){
        case 0x00:
            if(chip8->inst.NN == 0xE0){
                //0x00E0 Clear screen
                memset(&chip8->display[0],false,sizeof chip8->display);
            }else if(chip8->inst.NN== 0xEE){
                // Ox00EE return fromt the subroutine
                chip8->PC=(*--chip->stack_ptr);
            }
            break;
        
        case 0x02:
            //0x2NNN: call subroutine at NNN
            *chip8->stack_ptr++ =chip8->PC; //store current address
            chip8->PC=chip8->inst.NNN;   //set pc to subroutine memory location
            break 
        default:
            break; // unimplemented

    }
}


int main(int argc,char **argv){
    (void) argc;
    (void) argv;
    if(argc<2){
        fprintf(stderr,"Usage: %s <rom name>\n",argv[0]);
        exit(EXIT_FAILURE);
    }
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

    //Initialize the  CHIP8 machine
    chip8_t chip8={0};
    const char* rom_name=argv[1];
    if(!init_chip8(&chip8,rom_name)){
        exit(EXIT_FAILURE);
    }

    //Initial screen clear
    clear_screen(config,sdl);
  

    //Main emulator loop
    while(chip8.state != QUIT){
        // handle inputs
        handle_input(&chip8);

        if(chip8.state==PAUSED) continue;
        //Emulate chip instruction
        emulate_instruction(&chip8);
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