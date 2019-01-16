# compilation
git clone --recurse-submodules https://github.com/ssloy/repdvis.git  
cd repdvis  
mkdir build  
cd build  
cmake ..  
make  



# Attention, à la FST le cmake est trop ancien ; il faut modifier le fichier lib/glfw/CMakeLists.txt :
```
diff --git a/CMakeLists.txt b/CMakeLists.txt
index 4141629..09ffb80 100644
--- a/CMakeLists.txt
+++ b/CMakeLists.txt
@@ -1,4 +1,5 @@
-cmake_minimum_required(VERSION 3.0)
+cmake_minimum_required(VERSION 2.8)
+cmake_policy(SET CMP0022 NEW)
 
 project(GLFW C)
 ```
