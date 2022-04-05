# APVIS

## Description

Visualizer for shairport-sync


## build instructions

Clone and build the project:

- install gtkmm library, gtk development library
- install pulse audio development library
- install conan (for regenerating conan dependencies, go to conan subdirectory and execute "conan install ..")
- install opengl libraries
- build with cmake or with cmake compatible IDE

## Running the application

- Start shairport-sync:
  - using pulse audio as backend
  - with metadata provided as pipe
- Start this application