cmake_minimum_required(VERSION 3.18)
project(Hydronaut CXX)

# ─── C++ Standard ─────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─── LibTorch ─────────────────────────────────────────────────────────────────
# Point CMAKE_PREFIX_PATH to the libtorch directory, e.g.:
#   cmake -B build -DCMAKE_PREFIX_PATH=./libtorch
find_package(Torch REQUIRED)

# LibTorch requires its own CXX flags (ABI selection, etc.)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

# ─── SFML ─────────────────────────────────────────────────────────────────────
find_package(SFML 2.5 COMPONENTS graphics window system audio REQUIRED)

# ─── Shared game library ──────────────────────────────────────────────────────
# All game source files EXCEPT main executables.
add_library(hydronaut_lib STATIC
    src/AssetManager.cpp

    src/Particle.cpp
    src/StarfishObstacle.cpp
    src/SineObstacle.cpp
    src/ParabolicObstacle.cpp
    src/SecObstacle.cpp
    src/ExpSineObstacle.cpp
    src/Treasure.cpp
    src/SimulationEnvironment.cpp
    src/Level1.cpp
    src/Level2.cpp
    src/Level3.cpp
    src/Menu.cpp
    src/HumanTrainer.cpp
    src/CommandQueue.cpp
    src/EventBus.cpp
    src/Submarine.cpp
    src/EntityState.cpp
    src/RewardStrategy.cpp
)

target_include_directories(hydronaut_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${TORCH_INCLUDE_DIRS}
)

target_link_libraries(hydronaut_lib PUBLIC
    sfml-graphics
    sfml-window
    sfml-system
    sfml-audio
    ${TORCH_LIBRARIES}
)

target_compile_options(hydronaut_lib PRIVATE
    -Wall -Wextra -O2 -Wno-deprecated-declarations
)

# ─── Original game executable ─────────────────────────────────────────────────
add_executable(hydronaut
    src/main.cpp
)
target_link_libraries(hydronaut PRIVATE hydronaut_lib)

# ─── train_hydronaut — headless RL training ───────────────────────────────────
add_executable(train_hydronaut src/train.cpp)
target_link_libraries(train_hydronaut PRIVATE hydronaut_lib)

# ─── play_hydronaut — SFML inference visualiser ───────────────────────────────
add_executable(play_hydronaut src/play.cpp)
target_link_libraries(play_hydronaut PRIVATE hydronaut_lib)

# ─── RPATH: embed LibTorch shared-lib path so binaries find libtorch at runtime ─
# This means you do NOT need LD_LIBRARY_PATH when running from the build dir.
set_target_properties(train_hydronaut play_hydronaut hydronaut PROPERTIES
    BUILD_RPATH  "${TORCH_INSTALL_PREFIX}/lib"
    INSTALL_RPATH "${TORCH_INSTALL_PREFIX}/lib"
)

# ─── Summary ──────────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "────────────────────────────────────────────────")
message(STATUS "  Hydronaut RL build configuration")
message(STATUS "  Torch root  : ${TORCH_INSTALL_PREFIX}")
message(STATUS "  SFML libs   : sfml-graphics sfml-window sfml-system sfml-audio")
message(STATUS "  Targets     : hydronaut | train_hydronaut | play_hydronaut")
message(STATUS "────────────────────────────────────────────────")
message(STATUS "")
message(STATUS "  Quick-start:")
message(STATUS "    cmake -B build -DCMAKE_PREFIX_PATH=./libtorch")
message(STATUS "    cmake --build build -j\$(nproc)")
message(STATUS "    cd build && ./train_hydronaut")
message(STATUS "    cd build && ./play_hydronaut 2   # or 1 or 3")
message(STATUS "")

# ─── Mock Executable ──────────────────────────────────────────────────────────
add_executable(mock_memento src/mock_memento.cpp)
target_link_libraries(mock_memento PRIVATE hydronaut_lib)
