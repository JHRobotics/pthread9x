/* 
  WARNING WARNING: this only fix link, if refernce on symbol is present
  on static library, but this object is not referenced or not ever called
  (LTO usualy remove this references)

  If this function will be called, it leads to failure!
*/
DWORD _imp___fstat64 = 0;
DWORD _imp___wstat64 = 0;
