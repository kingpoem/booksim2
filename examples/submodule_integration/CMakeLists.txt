cmake_minimum_required(VERSION 3.25)
project(BookSim2SubmoduleDemo LANGUAGES CXX)

add_subdirectory(../../ external_booksim2)

add_executable(booksim_embed_demo main.cpp)
target_link_libraries(booksim_embed_demo PRIVATE Booksim::booksim_core)
target_compile_features(booksim_embed_demo PRIVATE cxx_std_20)
