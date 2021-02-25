#ifndef bird_file_h
#define bird_file_h

#include "native.h"
#include "object.h"

#define DECLARE_FILE_METHOD(name) DECLARE_METHOD(file##name)

/**
 * file(path: string [, mode: string])
 *
 * opens a new file handle to the file specified
 * available modes are as exists in C, with the following exception
 * - w+ does not tructuate the file, but will still create it
 *   if it does not exist
 * - default mode is 'r' i.e. read only
 */
DECLARE_NATIVE(file);

/**
 * file.exists()
 *
 * returns true if a file exists or false otherwise
 */
DECLARE_FILE_METHOD(exists);

/**
 * file.close()
 *
 * closes the stream to a file.
 * you will rarely ever need to call this method.
 * it's provided as a just-in-case mechanism
 * - we've had our own share of stubborn files :)
 */
DECLARE_FILE_METHOD(close);

/**
 * file.open()
 *
 * opens the stream to a file for the operation specified in the file
 * constructor.
 * you will need to call this method after a call to read or write if you wish
 * to read or write again as the file will already be closed.
 */
DECLARE_FILE_METHOD(open);

/**
 * file.is_open()
 *
 * returns true if a file is open for reading or writing; false otherwise.
 * _NOTE_: std files are always open
 */
DECLARE_FILE_METHOD(is_open);

/**
 * file.is_closed()
 *
 * returns true if a file is closed for reading or writing; false otherwise.
 * _NOTE_: std files are never closed
 */
DECLARE_FILE_METHOD(is_closed);

/**
 * file.read([size: number])
 *
 * reads the contents of an opened file and return it as string or bytes when
 * opened in binary mode
 * - this requires mode 'r' (which is the default) on the file
 * - when reading binary files (mode 'b'), you will have to close it yourself if
 * you didn't read the entire file
 */
DECLARE_FILE_METHOD(read);

/**
 * file.write(data: string)
 *
 * writes a string or bytes to an opened file.
 * - this requires mode 'w', 'a' or 'r+'
 * - when writing binary (mode 'b'), you will have to close it yourself if you
 */
DECLARE_FILE_METHOD(write);

/**
 * file.number()
 *
 * return the integer file descriptor that is used by the underlying
 * implementation to request I/O operations from the operating system
 *
 * this can be useful for other, lower level interfaces that use
 * file descriptors
 */
DECLARE_FILE_METHOD(number);

/**
 * file.is_tty()
 *
 * returns true if the file is connected to a tty-like
 * device or false otherwise
 */
DECLARE_FILE_METHOD(is_tty);

/**
 * file.flush()
 *
 * flushes the buffer held by a file.
 * this could be useful for writable files as file
 * writes are buffered.
 */
DECLARE_FILE_METHOD(flush);

/**
 * file.stats()
 *
 * returns the statistic/details of a file
 */
DECLARE_FILE_METHOD(stats);

/**
 * file.symlink(path: string)
 */
DECLARE_FILE_METHOD(symlink);

/**
 * file.delete()
 *
 * deletes a file
 * - if one or more processes have the file open when delete() is called, the
 * file will not be deleted until the last process frees it
 * - file.delete() throws an error if it fails
 */
DECLARE_FILE_METHOD(delete);

/**
 * file.rename(new_name: string)
 *
 * renames a file to new_name
 * - new_name cannot be empty
 * - if new name is in a different path, the file is moved
 * - file.rename() throws an error if it fails
 */
DECLARE_FILE_METHOD(rename);

/**
 * file.path()
 *
 * returns the path to a file
 */
DECLARE_FILE_METHOD(path);

/**
 * file.mode()
 *
 * returns the current opened mode of a file
 */
DECLARE_FILE_METHOD(mode);

/**
 * file.name()
 *
 * returns name of the current file
 */
DECLARE_FILE_METHOD(name);

/**
 * file.abs_path()
 *
 * returns the absolute path to a file
 */
DECLARE_FILE_METHOD(abs_path);

/**
 * file.copy(path: string)
 *
 * copies a file from a specified path to another
 * - it follows the mode in which the file was opened
 */
DECLARE_FILE_METHOD(copy);

/**
 * file.truncate([length: number])
 *
 * - truncates an entire file if no argument is given
 * - truncates a file to length if given
 */
DECLARE_FILE_METHOD(truncate);

/**
 * file.chmod(mode: number)
 *
 * changes the permission on a file to the specified number
 * - this number can be a series or ored FilePerm items
 */
DECLARE_FILE_METHOD(chmod);

/**
 * file.set_times(atime: number, mtime: number)
 *
 * sets the modification and last access time of a file
 * - time expected in utc seconds
 * - set argument to -1 to leave the current value
 */
DECLARE_FILE_METHOD(set_times);

/**
 * file.seek(position: number, seek_type: number)
 *
 * set's the position of a file reader or writer in a file.
 * - position must be within the range of the file size
 * - seek_type must be one of:
 *    - io.SEEK_SET (0)
 *    - io.SEEK_CUR (1)
 *    - io.SEEK_END (2)
 */
DECLARE_FILE_METHOD(seek);

/**
 * f.tell()
 *
 * returns the current position of the reader/writer in a file
 */
DECLARE_FILE_METHOD(tell);

#endif