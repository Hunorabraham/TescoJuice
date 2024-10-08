cd %~dp0
g++ -c source.cpp -I D:\\OpenGLincludes -I D:\\repos\\TJGNU
g++ -mwindows -o build\\out.exe glad.o stbGlue.o source.o -I D:\\OpenGLincludes -I D:\\repos\\TJGNU D:\\OpenGLlibs\\libglfw3dll.a -lgdi32
exit