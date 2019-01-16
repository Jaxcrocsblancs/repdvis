# compilation
git clone --recurse-submodules https://github.com/ssloy/repdvis.git  
cd repdvis  
mkdir build  
cd build  
cmake ..  
make  



# Attention, à la FST le cmake est trop ancien ; il faut modifier le fichier lib/glfw/CMakeLists.txt:
cmake_minimum_required(VERSION 2.8)  
cmake_policy(SET CMP0022 NEW)  
