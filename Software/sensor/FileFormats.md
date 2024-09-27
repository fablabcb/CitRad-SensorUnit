# RAW File Format

## Header

* 2 bytes (uint16): File Format Version
* 4 bytes (time_t): Timestamp of file creation
* 2 bytes (uint16): number of fft bins
* 1 byte  (uint_8): size of each frequency value (1 or 4 bytes) -> D_SIZE
* 1 byte  (bool):   use iq
* 2 bytes (uint16): sample rate

## Chunk Data

For each FFT a single chunk of data is written.

* 4 bytes (uint32): Milliseconds the sensor has been running
* 4 bytes (uint32): number of values to read next -> NUM
* if in 8bit mode (Header:D_SIZE is 1): NUM bytes (uint_8): frequency values
* if not in 8bit mode (Header:D_SIZE is 4): 4 x NUM bytes (NUM x float): frequency values
* 4 bytes (0xffffffff): chunk end marker; all ones; NaN as float

## Error Correction

None.

When reading, check for the end marker at the end of chunk data. If it is not all 1s, discard the last chunk and
search for the next end marker. Start reading again after that one.


