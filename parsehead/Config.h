#define ROOT_DIR "C:/Users/zdh/Documents/GitHub/Smile_Simulation"
#define SHADER_DIR "C:/Users/zdh/Documents/GitHub/Smile_Simulation/parsehead"
#define MESH_DIR "C:/Users/zdh/Documents/GitHub/Smile_Simulation/parsehead/face_models"
#define TEX_DIR "C:/Users/zdh/Documents/GitHub/Smile_Simulation/parsehead/texture"
#define CAN_DIR "C:/Users/zdh/Documents/GitHub/Smile_Simulation/parsehead/split"
#define PF_WIN32 1
#define PF_LINUX 2

#ifdef _WIN32
#define PLATFORM_FLAG PF_WIN32
#elif __linux__
#define PLATFORM_FLAG PF_LINUX
#endif
