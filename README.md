# bdf2h
Create a C header file containing data from a bdf monospace font file

Usage: bdf2h input_bdf_file output_h_file max_character_encoding

max_character_encoding permit to stop before the end of file.
It is not the number of characters read, but the encoding number at which to stop.
It fills the non declared characters with blank chars, so if you specify 128 at the
max_encoding_number, you will have an array of 128 characters in the output array,
even if there are some blanks.

Note: it does not handle non monospace fonts, or fonts with a horizontal size > 8
