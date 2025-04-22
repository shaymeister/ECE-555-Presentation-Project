#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable

__kernel void SimpleCopy(__global uchar *ImgDst, __global uchar *ImgSrc, uint Hpixels, uint VPixels)
{
    // Perform a pixel wise copy across all pixels in the image 
    //
    // Arguments:
    // ----------
    // ImgDst (uchar pointer): the location to store the copied pixels
    // ImgSrc (uchar pointer): the location to the the pixel values
    // Hpixels (uint): the number of horizontal pixels
    // VPixels (uint): the number of vertical pixels
    //
    // Returns:
    // --------
    // void
    uint idx = get_global_id(0);  // Get the global thread ID
    ImgDst[idx] = ImgSrc[idx];    // Copy data from ImgSrc to ImgDst
}

__kernel void Vflip(__global uchar* ImgDst,
                    __global uchar* ImgSrc,
                    const uint Hpixels,
                    const uint Vpixels)
{
    // Perform a pixel wise vertical image flip all pixels in the image 
    //
    // Arguments:
    // ----------
    // ImgDst (uchar pointer): the location to store the flipped pixels
    // ImgSrc (uchar pointer): the location to the the pixel values
    // Hpixels (uint): the number of horizontal pixels
    // VPixels (uint): the number of vertical pixels
    //
    // Returns:
    // --------
    // void
    uint ThrPerBlk = get_local_size(0);
    uint MYbid = get_group_id(0);
    uint MYtid = get_local_id(0);
    uint MYgtid = ThrPerBlk * MYbid + MYtid;

    uint BlkPerRow = (Hpixels + ThrPerBlk - 1) / ThrPerBlk; // ceil
    uint RowBytes = (Hpixels * 3 + 3) & (~3);
    uint MYrow = MYbid / BlkPerRow;
    uint MYcol = MYgtid - MYrow * BlkPerRow * ThrPerBlk;

    if (MYrow >= Vpixels || MYcol >= Hpixels)
        return;

	uint MYmirrorrow = Vpixels - 1 - MYrow;
	uint MYsrcOffset = MYrow * RowBytes;
	uint MYdstOffset = MYmirrorrow * RowBytes;
	uint MYsrcIndex = MYsrcOffset + 3 * MYcol;
	uint MYdstIndex = MYdstOffset + 3 * MYcol;

    ImgDst[MYdstIndex] = ImgSrc[MYsrcIndex];
    ImgDst[MYdstIndex + 1] = ImgSrc[MYsrcIndex + 1];
    ImgDst[MYdstIndex + 2] = ImgSrc[MYsrcIndex + 2];
}

__kernel void Hflip(__global uchar* ImgDst,
                    __global uchar* ImgSrc,
                    const uint Hpixels,
                    const uint Vpixels)
{
    // Perform a pixel wise horizontal image flip all pixels in the image 
    //
    // Arguments:
    // ----------
    // ImgDst (uchar pointer): the location to store the flipped pixels
    // ImgSrc (uchar pointer): the location to the the pixel values
    // Hpixels (uint): the number of horizontal pixels
    // VPixels (uint): the number of vertical pixels
    //
    // Returns:
    // --------
    // void
    uint ThrPerBlk = get_local_size(0);
    uint MYbid = get_group_id(0);
    uint MYtid = get_local_id(0);
    uint MYgtid = ThrPerBlk * MYbid + MYtid;

    uint BlkPerRow = (Hpixels + ThrPerBlk - 1) / ThrPerBlk; // ceil
    uint RowBytes = (Hpixels * 3 + 3) & (~3);
    uint MYrow = MYbid / BlkPerRow;
    uint MYcol = MYgtid - MYrow * BlkPerRow * ThrPerBlk;

    if (MYrow >= Vpixels || MYcol >= Hpixels)
        return;

    uint MYmirrorcol = Hpixels - 1 - MYcol;
    uint MYoffset = MYrow * RowBytes;
    uint MYsrcIndex = MYoffset + 3 * MYcol;
    uint MYdstIndex = MYoffset + 3 * MYmirrorcol;

    ImgDst[MYdstIndex] = ImgSrc[MYsrcIndex];
    ImgDst[MYdstIndex + 1] = ImgSrc[MYsrcIndex + 1];
    ImgDst[MYdstIndex + 2] = ImgSrc[MYsrcIndex + 2];
}
