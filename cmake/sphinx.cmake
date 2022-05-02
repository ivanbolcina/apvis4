
    # set input and output files
    set(SPHINX_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs_sphinx/conf.py)
    set(SPHINX_OUT ${CMAKE_CURRENT_BINARY_DIR}/conf.py)

    # request to configure the file
    configure_file(${SPHINX_IN} ${SPHINX_OUT} @ONLY)
    message("Sphinx build started")

    # Note: do not put "ALL" - this builds docs together with application EVERY TIME!
    add_custom_target( docs
            COMMAND make html
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs_sphinx
            COMMENT "Generating API documentation with Sphinx"
            VERBATIM )
