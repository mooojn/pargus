cmake_minimum_required(VERSION 3.16)

project(Pargus C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_executable(Pargus
    main.c
    common/string_utils.c
    common/timing.c
    config/args.c
    io/reader.c
    io/writer.c
    parallel/doc_queue.c
    parallel/work_queue.c
    nlp/stopwords.c
    nlp/tokenizer.c
    nlp/tfidf.c
    nlp/minhash.c
    nlp/lsh.c
    nlp/similarity.c
    nlp/ngram.c
)

target_include_directories(Pargus PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(OpenMP)
if(OpenMP_C_FOUND)
    target_link_libraries(Pargus PRIVATE OpenMP::OpenMP_C)
    target_compile_definitions(Pargus PRIVATE PARGUS_HAS_OPENMP=1)
endif()

find_package(Threads REQUIRED)
target_link_libraries(Pargus PRIVATE Threads::Threads)

if(UNIX)
    target_link_libraries(Pargus PRIVATE m)
endif()

enable_testing()

add_executable(test_tokenizer
    tests/test_tokenizer.c
    common/string_utils.c
    nlp/stopwords.c
    nlp/tokenizer.c
)
target_include_directories(test_tokenizer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME tokenizer COMMAND test_tokenizer)

add_executable(test_tfidf
    tests/test_tfidf.c
    common/string_utils.c
    config/args.c
    nlp/stopwords.c
    nlp/tokenizer.c
    nlp/tfidf.c
)
target_include_directories(test_tfidf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(UNIX)
    target_link_libraries(test_tfidf PRIVATE m)
endif()
add_test(NAME tfidf COMMAND test_tfidf)

add_executable(test_minhash_lsh
    tests/test_minhash_lsh.c
    common/string_utils.c
    config/args.c
    nlp/stopwords.c
    nlp/tokenizer.c
    nlp/tfidf.c
    nlp/minhash.c
    nlp/lsh.c
)
target_include_directories(test_minhash_lsh PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(UNIX)
    target_link_libraries(test_minhash_lsh PRIVATE m)
endif()
add_test(NAME minhash_lsh COMMAND test_minhash_lsh)

add_executable(test_similarity
    tests/test_similarity.c
    common/string_utils.c
    config/args.c
    nlp/stopwords.c
    nlp/tokenizer.c
    nlp/tfidf.c
    nlp/lsh.c
    nlp/similarity.c
)
target_include_directories(test_similarity PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(UNIX)
    target_link_libraries(test_similarity PRIVATE m)
endif()
add_test(NAME similarity COMMAND test_similarity)

add_executable(test_writer_escape
    tests/test_writer_escape.c
    common/string_utils.c
    common/timing.c
    config/args.c
    io/writer.c
    nlp/similarity.c
)
target_include_directories(test_writer_escape PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(UNIX)
    target_link_libraries(test_writer_escape PRIVATE m)
endif()
add_test(NAME writer_escape COMMAND test_writer_escape)

if(MSVC)
    target_compile_options(Pargus PRIVATE /W4)
else()
    target_compile_options(Pargus PRIVATE -Wall -Wextra -Wpedantic)
endif()
