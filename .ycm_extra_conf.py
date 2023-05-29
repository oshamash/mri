# This file is NOT licensed under the GPLv3, which is the license for the rest
# of YouCompleteMe.
#
# Here's the license text for this file:
#
# This is free and unencumbered software released into the public domain.
#
# Anyone is free to copy, modify, publish, use, compile, sell, or
# distribute this software, either in source code form or as a compiled
# binary, for any purpose, commercial or non-commercial, and by any
# means.
#
# In jurisdictions that recognize copyright laws, the author or authors
# of this software dedicate any and all copyright interest in the
# software to the public domain. We make this dedication for the benefit
# of the public at large and to the detriment of our heirs and
# successors. We intend this dedication to be an overt act of
# relinquishment in perpetuity of all present and future rights to this
# software under copyright law.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
# For more information, please refer to <http://unlicense.org/>

from distutils.sysconfig import get_python_inc
import os
import platform
import os.path as p
import subprocess
import json
import re
import logging

logger = logging.getLogger('ycm-extra-conf')

DIR_OF_THIS_SCRIPT = p.abspath( p.dirname( __file__ ) )
DIR_OF_THIRD_PARTY = p.join( DIR_OF_THIS_SCRIPT, 'third_party' )
SOURCE_EXTENSIONS = [ '.cpp', '.cxx', '.cc', '.c', '.m', '.mm' ]

# These are the compilation flags that will be used in case there's no
# compilation database set (by default, one is not set).
# CHANGE THIS LIST OF FLAGS. YES, THIS IS THE DROID YOU HAVE BEEN LOOKING FOR.
flags = [
'-Wall',
'-Wextra',
'-Werror',
'-Wno-long-long',
'-Wno-variadic-macros',
'-fexceptions',
'-DNDEBUG',
# You 100% do NOT need -DUSE_CLANG_COMPLETER and/or -DYCM_EXPORT in your flags;
# only the YCM source code needs it.
'-DUSE_CLANG_COMPLETER',
'-DYCM_EXPORT=',
# Custome flags
'-Iinclude',
'-isystem',
'cpp/pybind11',
'-isystem',
'cpp/whereami',
'-isystem',
'cpp/BoostParts',
'-isystem',
get_python_inc(),
'-isystem',
'cpp/llvm/include',
'-isystem',
'cpp/llvm/tools/clang/include',
'-I',
'cpp/ycm',
'-I',
'cpp/ycm/ClangCompleter',
'-isystem',
'cpp/ycm/tests/gmock/gtest',
'-isystem',
'cpp/ycm/tests/gmock/gtest/include',
'-isystem',
'cpp/ycm/tests/gmock',
'-isystem',
'cpp/ycm/tests/gmock/include',
'-isystem',
'cpp/ycm/benchmarks/benchmark/include',
]


# Set this to the absolute path to the folder (NOT the file!) containing the
# compile_commands.json file to use that instead of 'flags'. See here for
# more details: http://clang.llvm.org/docs/JSONCompilationDatabase.html
#
# You can get CMake to generate this file for you by adding:
#   set( CMAKE_EXPORT_COMPILE_COMMANDS 1 )
# to your CMakeLists.txt file.
#
# Most projects will NOT need to set this to anything; you can just change the
# 'flags' list of compilation flags. Notice that YCM itself uses that approach.

def IsHeaderFile( filename ):
  extension = p.splitext( filename )[ 1 ]
  return extension in [ '.h', '.hxx', '.hpp', '.hh' ]


def FindCorrespondingSourceFile( filename ):
  if IsHeaderFile( filename ):
    basename = p.splitext( filename )[ 0 ]
    for extension in SOURCE_EXTENSIONS:
      replacement_file = basename + extension
      if p.exists( replacement_file ):
        return replacement_file
  return filename


def PathToPythonUsedDuringBuild():
  try:
    filepath = p.join( DIR_OF_THIS_SCRIPT, 'PYTHON_USED_DURING_BUILDING' )
    with open( filepath ) as f:
      return f.read().strip()
  except OSError:
    return None

def dirwalk_up(bottom):
    """
    mimic os.walk, but walk 'up'
    instead of down the directory tree
    """

    bottom = p.realpath(bottom)

    #get files in current dir
    try:
        names = os.listdir(bottom)
    except Exception as e:
        print(e)
        return


    dirs = []
    files = []
    for name in names:
        if p.isdir(p.join(bottom, name)):
            dirs.append(name)
        elif p.isfile(p.join(bottom, name)):
            files.append(name)

    yield bottom, dirs, files

    new_path = p.realpath(p.join(bottom, '..'))

    # see if we are at the top
    if new_path == bottom:
        return

    for x in dirwalk_up(new_path):
        yield x

def FindNearBuildPath(filename):
    file_dir = p.dirname(filename)
    for path, dirs, files in dirwalk_up(file_dir):
        if "compile_comands.json" in files:
            return path
        if path == os.getcwd():
            return None
        for rdir in dirs:
            if rdir == "build" and p.exists(rdir + "/compile_commands.json"):
                return "{}/build".format(path)
    return None


def Settings( **kwargs ):
  # Do NOT import ycm_core at module scope.
  import ycm_core

  language = kwargs[ 'language' ]

  if language == 'cfamily':
    # If the file is a header, try to find the corresponding source file and
    # retrieve its flags from the compilation database if using one. This is
    # necessary since compilation databases don't have entries for header files.
    # In addition, use this source file as the translation unit. This makes it
    # possible to jump from a declaration in the header file to its definition
    # in the corresponding source file.
    filename = FindCorrespondingSourceFile( kwargs[ 'filename' ] )

    logger.info("Calculating flags for {}".format(filename))

    # If we can't find compilation DB -> fallback to flags
    compilation_database_folder = FindNearBuildPath(filename)
    if not compilation_database_folder:
        logger.info("Could not find build-path, using default falgs")
        cpy_flags = flags
        if any([filename.endswith(x) for x in ("c", "cc", "h", "hh")]):
            cpy_flags += ['-x', 'c', '-std=c11']
        elif any([filename.endswith(x) for x in ("cpp", "cxx", "hpp", "hxx")]):
            cpy_flags += ['-x', 'c++', '-std=c++17']
        return {
          'flags': cpy_flags,
          'include_paths_relative_to_dir': DIR_OF_THIS_SCRIPT,
          'override_filename': filename
        }

    try:
        logger.info("Found build-path, opening {}/compile_commands.json".format(compilation_database_folder))
        db_file = open(compilation_database_folder + "/compile_commands.json")
        database = json.load(db_file)
        entry = [e for e in database if e['file'].endswith(filename)]
        if not entry:
            logger.info("No entry for {} in compilation-db".format(filename))
            raise Exception("No entry")
        logger.info("Found entry for {} in compilation-db".format(filename))
        entry = entry[0]
        entry_fname = entry['file']
        entry_command = entry['command']
        entry_directory = entry['directory']

        iterable = iter(entry_command.split())
        final_flags = [] + RTE_INCLUDES
        try:
            current = next(iterable)

            # Skip until flags
            clang_compilers = ("clang")
            c_compilers = ("gcc", "c", "cc")
            cpp_compilers = ("g++", "c++")
            while True:
                # already a flag - keep it
                if current.startswith('-'):
                    break
                # Skip filename
                if current == entry_fname:
                    current = next(iterable)
                    break
                # c-compiler, use -x flag to indicate clangd we are handling C file
                if current in c_compilers or any([current.endswith("/" + c) for c in c_compilers]):
                    final_flags += ['-x', 'c']
                    current = next(iterable)
                    break
                # cpp-compiler, use -x flag to indicate clangd we are handling C++ file
                elif current in cpp_compilers or any([current.endswith("/" + c) for c in cpp_compilers]):
                    final_flags += ['-x', 'c++']
                    current = next(iterable)
                    break
                # for clang, it should already contain the -x flag
                elif current in clang_compilers or any([current.endswith("/" + c) for c in clang_compilers]):
                    current = next(iterable)
                    break
                current = next(iterable)


            while True:
                # Defines, includes can be multi-param
                if current in ("-D", "-I"):
                    final_flags += [current.append(next(iterable))]
                # Double-skip these flags (-c file, -o file)
                elif current in ("-o", "-c"):
                    current = next(iterable)
                # Normal flag
                else:
                    final_flags += [current]
                current = next(iterable)


        except StopIteration:
            pass

        logger.info("Successfully loaded flags from compilation-db")
        return {
          'flags': final_flags,
          'include_paths_relative_to_dir': relative_dir,
          'override_filename': filename
        }
    except:
        logger.warn("Using default flags")
        cpy_flags = flags
        if any([filename.endswith(x) for x in ("c", "cc", "h", "hh")]):
            cpy_flags += ['-x', 'c', '-std=c11']
        elif any([filename.endswith(x) for x in ("cpp", "cxx", "hpp", "hxx")]):
            cpy_flags += ['-x', 'c++', '-std=c++17']
        return {
          'flags': cpy_flags,
          'include_paths_relative_to_dir': DIR_OF_THIS_SCRIPT,
          'override_filename': filename
        }

  if language == 'python':
    return {
      'interpreter_path': PathToPythonUsedDuringBuild()
    }

  return {}


def PythonSysPath( **kwargs ):
  sys_path = kwargs[ 'sys_path' ]

  interpreter_path = kwargs[ 'interpreter_path' ]
  major_version = subprocess.check_output( [
    interpreter_path, '-c', 'import sys; print( sys.version_info[ 0 ] )' ]
  ).rstrip().decode( 'utf8' )

  sys_path[ 0:0 ] = [ p.join( DIR_OF_THIS_SCRIPT ),
                      p.join( DIR_OF_THIRD_PARTY, 'bottle' ),
                      p.join( DIR_OF_THIRD_PARTY, 'cregex',
                              'regex_{}'.format( major_version ) ),
                      p.join( DIR_OF_THIRD_PARTY, 'frozendict' ),
                      p.join( DIR_OF_THIRD_PARTY, 'jedi_deps', 'jedi' ),
                      p.join( DIR_OF_THIRD_PARTY, 'jedi_deps', 'parso' ),
                      p.join( DIR_OF_THIRD_PARTY, 'requests_deps', 'requests' ),
                      p.join( DIR_OF_THIRD_PARTY, 'requests_deps',
                                                  'urllib3',
                                                  'src' ),
                      p.join( DIR_OF_THIRD_PARTY, 'requests_deps',
                                                  'chardet' ),
                      p.join( DIR_OF_THIRD_PARTY, 'requests_deps',
                                                  'certifi' ),
                      p.join( DIR_OF_THIRD_PARTY, 'requests_deps',
                                                  'idna' ),
                      p.join( DIR_OF_THIRD_PARTY, 'waitress' ) ]

  sys_path.append( p.join( DIR_OF_THIRD_PARTY, 'jedi_deps', 'numpydoc' ) )
  return sys_path
