
#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#else
typedef unsigned short uint16_t;
#endif

#include <errno.h>
#include <stdio.h> // for NULL

#include "CFAReader.h"

CFAReader::CFAReader()
#if defined(_WIN32) || defined(_WIN64)
: m_buf(NULL),
  m_fd(INVALID_HANDLE_VALUE),
#else
: m_buf((uint8_t*)MAP_FAILED),
  m_fd(-1),
#endif
  m_filesz(0),
  m_curr_byte(NULL),
  m_byte_aligned(true)
{
}

CFAReader::~CFAReader()
{
#if defined(_WIN32) || defined(_WIN64)
	if (m_buf != NULL)
	        UnmapViewOfFile(m_buf);
	if (m_map_handle != NULL)
		CloseHandle(m_map_handle);
	if (m_fd != INVALID_HANDLE_VALUE)
		CloseHandle(m_fd);
#else
	if (m_buf != MAP_FAILED) {
		::munmap(m_buf, m_filesz);
		m_buf = NULL;
	}

	if (m_fd != -1) {
		::close(m_fd);
		m_fd = -1;
	}
#endif
}

int CFAReader::open(const char* fname, size_t expected_size)
{
#if defined(_WIN32) || defined(_WIN64)
	m_fd = CreateFile(fname, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == m_fd)
		return GetLastError();
	m_filesz = GetFileSize(m_fd, NULL);
	if (INVALID_FILE_SIZE == m_filesz)
		return GetLastError();
	if (m_filesz != expected_size)
		return EINVAL;
	m_map_handle = CreateFileMapping(m_fd, NULL, PAGE_READONLY, 0, m_filesz, NULL);
	if (NULL == m_map_handle)
		return GetLastError();
	m_buf = (uint8_t *)MapViewOfFile(map_handle, FILE_MAP_READ, 0, 0, 0);
	if (NULL == m_buf)
		return GetLastError();
#else
	m_fd = ::open(fname, O_NOATIME | O_RDONLY);
	if (-1 == m_fd)
		return errno;

	struct stat stat;
	int rc = ::fstat(m_fd, &stat);
	if (-1 == rc)
		return errno;

	m_filesz = stat.st_size;
	if (m_filesz != expected_size)
		return EINVAL;

	m_buf = (uint8_t *)::mmap(NULL, m_filesz, PROT_READ, MAP_PRIVATE | MAP_POPULATE, m_fd, 0);
	if (MAP_FAILED == m_buf)
		return errno;

#endif

	m_curr_byte = m_buf;

	return 0;
}

void CFAReader::read(uint8_t *out_buf)
{
	if (m_byte_aligned) {
		// layout: 22223333 XXXX1111
		out_buf[1] = m_curr_byte[1] & 0x0F;
		out_buf[0] = m_curr_byte[0];
		++m_curr_byte;
	} else {
		// layout: 3333XXXX 11112222
		out_buf[1] = m_curr_byte[1] >> 4;
		out_buf[0] = (m_curr_byte[1] << 4) | (m_curr_byte[0] >> 4);
		m_curr_byte += 2;
	}

	m_byte_aligned = !m_byte_aligned;
}

