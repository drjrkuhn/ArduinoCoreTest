#pragma once

#ifndef __SLIPSTREAM_H__
#define __SLIPSTREAM_H__

#include <cstdint>
#include <cassert>
#include <cstring>

namespace sproto {
	// for now, we are using human-readable escape and end characters rather than the SLIP default
	// for debugging
	constexpr uint8_t SLIP_END_CHAR = '#';      //= 0300;   // End of packet character;
	constexpr uint8_t SLIP_ESC_CHAR = '\\';     //= 0333;   // Escape character
	constexpr uint8_t SLIP_ESC_END_CHAR = 'X';  //= 0334;   // Escaped end character
	constexpr uint8_t SLIP_ESC_ESC_CHAR = 'E';  //= 0335;   // Escaped escape character


	constexpr uint8_t SLIP_ESC_END[]{ SLIP_ESC_CHAR, SLIP_ESC_END_CHAR };
	constexpr uint8_t SLIP_ESC_ESC[]{ SLIP_ESC_CHAR, SLIP_ESC_ESC_CHAR };

	typedef int error_t;

	constexpr error_t NO_ERROR = 0;  ///< no error
	constexpr error_t ERROR_TIMEOUT = -1; ///< stream timeout error
	constexpr error_t ERROR_BUFFER = -2; ///< stream buffer error
	constexpr error_t ERROR_STREAM = -3; ///< stream not ready error
	constexpr error_t ERROR_ENCODING = -4; ///< Protocol misread/miswrite error

	int writeSlipEscaped(uint8_t* dest, size_t dest_size, size_t& ndest, const uint8_t* src, size_t src_size, size_t& nsrc) {
		uint8_t* enddest = dest;
		const uint8_t* endsrc = src;
		ndest = 0; // total destination buffer characters written
		nsrc = 0; // total src buffer characters processed

		while (src_size--) {
			switch (endsrc[0]) {
			case SLIP_END_CHAR:
				if (0 < endsrc - src) {
					size_t chunklen = endsrc - src;
					if (ndest + chunklen < dest_size) {
						memcpy(dest, src, chunklen);
						nsrc += chunklen;
						ndest += chunklen;
						src += chunklen;
						dest += chunklen;
					} else {
						return ERROR_BUFFER;
					}
				}
				if (memcpy(SLIP_ESC_END, 2) == 2) {
					nsrc++; // processed one escape character
				}
				endsrc++; // skip escaped char
				src = endsrc;
				break;
			case SLIP_ESC_CHAR:
				if (0 < endsrc - src) {
					nsrc += writeBytes(src, endsrc - src);
				}
				if (writeBytes(SLIP_ESC_ESC, 2) == 2) {
					nsrc++; // processed one escape character
				}
				endsrc++; // skip escaped char
				src = endsrc;
				break;
			default:
				endsrc++;
			}
		}
		// write any remaining characters
		if (0 < endsrc - src) {
			nsrc += writeBytes(src, endsrc - src);
		}
		writeBytes(&SLIP_END_CHAR, 1);
		return nsrc;
	}

	/**
	 * @brief Base class for SLIP protocol communications
	 *
	 * @tparam D Derived class used for CRTP implementation of static polymorphism
	 */
	template <class D> // D is the derived type
	class SlipStream {
	protected:
		D& derived() { return *static_cast<D*>(this); }
		D const& derived() const { return *static_cast<D const*>(this); }

		// re-define in derived class. These instances should be unreachable. 
		size_t writeBytes_impl(const uint8_t* buffer, size_t size) { std::assert(false); return 0; };
		size_t readBytesUntil_impl(buffer, size, terminator, nread) { std::assert(false); return 0; }
		bool hasBytes_impl() { std::assert(false); return false; }
		void writeNow_impl() { std::assert(false); }
		void clearInput_impl() { std::assert(false); }
		bool isStreamReady_impl() { std::assert(false); }

	public:
		/**
		 * @brief Write SLIP escaped buffer.
		 *
		 * @param src       buffer to write
		 * @param src_size  size of buffer to write
		 * @return size_t   number of original un-escaped bytes written (NOT chars transmitted)
		 */
		size_t writeSlipEscaped(const uint8_t* src, size_t src_size) {
			if (!isStreamReady())
				return 0;
			const uint8_t* end = src;
			size_t ntx = 0; // total src buffer characters processed (NOT chars transmitted)

			while (src_size--) {
				switch (end[0]) {
				case SLIP_END_CHAR:
					if (0 < end - src) {
						ntx += writeBytes(src, end - src);
					}
					if (writeBytes(SLIP_ESC_END, 2) == 2) {
						ntx++; // processed one escape character
					}
					end++; // skip escaped char
					src = end;
					break;
				case SLIP_ESC_CHAR:
					if (0 < end - src) {
						ntx += writeBytes(src, end - src);
					}
					if (writeBytes(SLIP_ESC_ESC, 2) == 2) {
						ntx++; // processed one escape character
					}
					end++; // skip escaped char
					src = end;
					break;
				default:
					end++;
				}
			}
			// write any remaining characters
			if (0 < end - src) {
				ntx += writeBytes(src, end - src);
			}
			writeBytes(&SLIP_END_CHAR, 1);
			return ntx;
		}

		/**
		 * @brief UTF8 character version
		 */
		size_t writeSlipEscaped(const char* src, size_t src_size) {
			return writeSlipEscaped(reinterpret_cast<const uint8_t*>(src), src_size);
		}

		/**
		 * @brief Read SLIP escaped sequence from stream into buffer and remove escapes.
		 *
		 * Looks for standard SLIP END character.
		 *
		 * @param dest      destination buffer to fill
		 * @param dest_size size of destination buffer (should be large enough to read escaped stream)
		 * @param nread     number of bytes read after encoding
		 * @return
		 *  - ERROR_TIMEOUT timeout occurred before terminating character was found
		 *  - ERROR_BUFFER  read buffer too small
		 *  - ERROR_ENCODING slip stream was improperly encoded
		 *  - NO_ERROR      terminator found and read complete
		 */
		error_t readSlipEscaped(uint8_t* dest, size_t dest_size, size_t& nread) {
			if (!isStreamReady())
				return ERROR_STREAM;
			// leave room for SLIP_END at end of buffer
			error_t err = readBytesUntil(dest, dest_size - 1, SLIP_END_CHAR, nread);
			if (err != NO_ERROR) {
				return err;
			}
			if (nread == 0) {
				return ERROR_TIMEOUT;
			}
			uint8_t* src = dest;
			size_t remaining = nread;
			size_t nrx = 0;
			bool misread = false;
			while (remaining--) {
				if (src[0] == SLIP_ESC_CHAR) {
					if (remaining > 0 && src[1] == SLIP_ESC_END[1]) {
						dest[0] = SLIP_END_CHAR;
						src++;
					}
					else if (remaining > 0 && src[1] == SLIP_ESC_ESC[1]) {
						dest[0] = SLIP_ESC_CHAR;
						src++;
					}
					else {
						dest[0] = SLIP_ESC_CHAR;
						misread = true;
					}
				}
				else {
					dest[0] = src[0];
				}
				src++;
				dest++;
				nrx++;
			}
			nread = nrx;
			if (nrx == 0 || misread) {
				return ERROR_ENCODING;
			}
			return NO_ERROR;
		}

		/** @brief UTF8 character version */
		error_t readSlipEscaped(char* dest, size_t dest_size, size_t& nread) {
			return readSlipEscaped(reinterpret_cast<uint8_t*>(dest), dest_size, nread);
		}

		/**
		 * @brief Writes characters contained in buffer to stream.
		 *
		 * @param buffer buffer to write
		 * @param size size of buffer to write
		 * @returns number of characters written to the stream
		 */
		size_t writeBytes(const uint8_t* buffer, size_t size) {
			// return static_cast<D*>(this)->writeBytes_impl(buffer, size);
			return derived().writeBytes_impl(buffer, size);
		}

		/**
		 * @brief Read a string of bytes from the input UNTIL a terminator character is received
		 *  or a timeout occurs. The terminator character is NOT added to the end of the buffer.
		 *  timeout period is implementation defined.
		 *
		 * @param buffer    buffer to read. Buffer is not necessarily null terminated after read
		 * @param size      total size of buffer to read
		 * @param terminator terminator character to look for
		 * @param[out] nread number of characters read
		 * @returns
		 *  - ERROR_TIMEOUT timeout occurred before terminating character was found
		 *  - ERROR_BUFFER  read buffer too small
		 *  - NO_ERROR      terminator found and read complete
		 */
		error_t readBytesUntil(uint8_t* buffer, size_t size, char terminator, size_t& nread) {
			return derived().readBytesUntil_impl(buffer, size, terminator, nread);
		}

		/**
		 * @brief Does the stream have bytes in the receive buffer?
		 *
		 * @return true if input received
		 * @return false if no input detected
		 */
		bool hasBytes() {
			return derived().hasBytes_impl();
		}

		/**
		 * @brief flush (write) the contents of the transmit buffer immediately.
		 * Required by Teensy USB implementation if transmission is less than the
		 * 64 byte USB buffer size.
		 *
		 */
		void writeNow() {
			derived().writeNow_impl();
		}

		/**
		 * @brief clear (flush) the contents of the receive buffer immediately.
		 */
		void clearInput() {
			derived().clearInput_impl();
		}

		/**
		 * @brief Is the stream ready for transmission and reception? Usually set to
		 * true after startup
		 *
		 * @return true     stream is ready in both directions
		 * @return false    stream has not yet been started
		 */
		bool isStreamReady() {
			return derived().isStreamReady_impl();
		}


	};

}; // namespace sproto

#endif // #ifndef __SLIPSTREAM_H__