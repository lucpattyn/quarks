# - Check for the presence of CROW
#
# The following variables are set when CROW is found:
#  CROW_FOUND      = Set to true, if all components of CROW have been
#                         found.
#  CROW_INCLUDES   = Include path for the header files of CROW
#  CROW_LIBRARIES  = Link these to use CROW
#  CROW_LFLAGS     = Linker flags (optional)

if (NOT CROW_FOUND)

    if (NOT CROW_ROOT_DIR)
        set (CROW_ROOT_DIR ${CMAKE_INSTALL_PREFIX})
    endif (NOT CROW_ROOT_DIR)

    ##____________________________________________________________________________
    ## Check for the header files

    find_path (CROW_INCLUDES
            NAMES crow.h
            HINTS ${CROW_ROOT_DIR} ${CMAKE_INSTALL_PREFIX}
            PATH_SUFFIXES include
            )

    ##____________________________________________________________________________
    ## Actions taken when all components have been found

    find_package_handle_standard_args (CROW DEFAULT_MSG CROW_INCLUDES)

    if (CROW_FOUND)
        ## Update
        get_filename_component (CROW_ROOT_DIR ${CROW_INCLUDES} PATH)
        ## Feedback
        if (NOT CROW_FIND_QUIETLY)
            message (STATUS "Found components for CROW")
            message (STATUS "CROW_ROOT_DIR  = ${CROW_ROOT_DIR}")
            message (STATUS "CROW_INCLUDES  = ${CROW_INCLUDES}")
        endif (NOT CROW_FIND_QUIETLY)
    else (CROW_FOUND)
        if (CROW_FIND_REQUIRED)
            message (FATAL_ERROR "Could not find CROW!")
        endif (CROW_FIND_REQUIRED)
    endif (CROW_FOUND)

    ##____________________________________________________________________________
    ## Mark advanced variables

    mark_as_advanced (
            CROW_ROOT_DIR
            CROW_INCLUDES
    )

endif (NOT CROW_FOUND)