set(CRAWL_SRCS
    main.cc
)

add_executable(wcrawl ${CRAWL_SRCS})
target_link_libraries(wcrawl workflow)

add_subdirectory(tests)