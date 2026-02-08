# 1. Definir a versão mínima do CMake (O IDX usa versões recentes)
cmake_minimum_required(VERSION 3.10)

# 2. Nome do projeto (Obrigatório para tirar o erro que você recebeu)
project(InspectionEdgeNode VERSION 1.0)

# 3. Configurar C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 4. Incluir as pastas de cabeçalho (Onde estão seus .h)
include_directories(src/Core src/HAL src/Mocks)

# 5. Configurar o GoogleTest (Baixa automaticamente via internet no IDX)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 6. Criar o executável de Testes (O que você vai rodar no IDX)
add_executable(RunTests
    tests/main_test.cpp
    src/Core/EdgeProcessor.cpp
)

# 7. Ligar as bibliotecas de teste
target_link_libraries(RunTests GTest::gtest_main)

# 8. Habilitar descoberta de testes
include(GoogleTest)
gtest_discover_tests(RunTests)