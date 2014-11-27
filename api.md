                 ImageBase<ImplT>
                     /
                    /
                   /
                  /
   ImageBuf<T, Channels, Alloc>
                 /
                /
               /
              /
    Image<T, ColorSpace, Alloc>


TODO
  -Pixel class for faster and more convienent access.
  -Iterators
  -Fuzzy and direct comparisons.


ImageBase<T, Channels, Alloc>
  // Ownership queries.
  bool OwnsMemory() const;

  // Basic accessors.
  Width()
  Height()
  Channels()

  // Bounds checking.
  InBounds(x,y,c)
  InBounds(x,y)
  ClampToBounds(x,y,c)
  ClampToBounds(x,y)

  // Individual pixel setting.
  Set(x, y, c, value);
  Set(x, y, value*);

  // Individual pixel acess.
  T* Get(x,y,c)
  T* Get(x,y)

  // Setting pixels en-masse.
  SetAll(T* values);
  SetChannel(int channel, T value)

  // Row by row access.
  T* Row(int row);

  // Image windowing.
  ImageBase<T, Channels, Alloc> GetSubImage(Rectangle bounds);
  ImageBase<T, 1, Alloc> ExtractChannel(int channel);
  // TODO(cbraley): Is it possible to extract channels?

  // Image copying.
  bool CopyInto(ImageBase<T, Channels, Alloc>* other) const;
  bool CopyFrom(ImageBase<T, Channels, Alloc> other);


Image<T, ColorSpace, Alloc> : public ImageBuf<T, NumChannels, Alloc>
  
  















