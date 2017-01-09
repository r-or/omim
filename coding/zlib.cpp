#include "coding/zlib.hpp"

namespace coding
{
namespace
{
int ToInt(ZLib::Level level)
{
  switch (level)
  {
  case ZLib::Level::NoCompression: return Z_NO_COMPRESSION;
  case ZLib::Level::BestSpeed: return Z_BEST_SPEED;
  case ZLib::Level::BestCompression: return Z_BEST_COMPRESSION;
  case ZLib::Level::DefaultCompression: return Z_DEFAULT_COMPRESSION;
  }
}
}  // namespace

// ZLib::Processor ---------------------------------------------------------------------------------
ZLib::Processor::Processor(void const * data, size_t size) noexcept : m_init(false)
{
  // next_in is defined as z_const (see
  // http://www.zlib.net/manual.html).  Sometimes it's a const (when
  // ZLIB_CONST is defined), sometimes not, it depends on the local
  // zconf.h. So, for portability, const_cast<...> is used here, but
  // in any case, zlib does not modify |data|.
  m_stream.next_in = static_cast<unsigned char *>(const_cast<void *>(data));
  m_stream.avail_in = static_cast<unsigned int>(size);

  m_stream.next_out = m_buffer;
  m_stream.avail_out = kBufferSize;

  m_stream.zalloc = Z_NULL;
  m_stream.zfree = Z_NULL;
  m_stream.opaque = Z_NULL;
}

bool ZLib::Processor::ConsumedAll() const
{
  ASSERT(IsInit(), ());
  return m_stream.avail_in == 0;
}

bool ZLib::Processor::BufferIsFull() const
{
  ASSERT(IsInit(), ());
  return m_stream.avail_out == 0;
}

// ZLib::Deflate -----------------------------------------------------------------------------------
ZLib::DeflateProcessor::DeflateProcessor(void const * data, size_t size, ZLib::Level level) noexcept
  : Processor(data, size)
{
  int const ret = deflateInit(&m_stream, ToInt(level));
  m_init = (ret == Z_OK);
}

ZLib::DeflateProcessor::~DeflateProcessor() noexcept
{
  if (m_init)
    deflateEnd(&m_stream);
}

int ZLib::DeflateProcessor::Process(int flush)
{
  ASSERT(IsInit(), ());
  return deflate(&m_stream, flush);
}

// ZLib::Inflate -----------------------------------------------------------------------------------
ZLib::InflateProcessor::InflateProcessor(void const * data, size_t size) noexcept
  : Processor(data, size)
{
  int const ret = inflateInit(&m_stream);
  m_init = (ret == Z_OK);
}

ZLib::InflateProcessor::~InflateProcessor() noexcept
{
  if (m_init)
    inflateEnd(&m_stream);
}

int ZLib::InflateProcessor::Process(int flush)
{
  ASSERT(IsInit(), ());
  return inflate(&m_stream, flush);
}
}  // namespace coding
