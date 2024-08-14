#include <stdio.h>
#include <string.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <objidl.h>
#include "ncbind/ncbind.hpp"
#include <map>
#include <vector>
#include <iostream>
#include <string>
#include <sstream>

#include <stdio.h>
#include <stdlib.h>

#include "tp_stub.h"

#include "XP3Archive.h"
#include "UtilStreams.h"
#include "tjsUtils.h"
#include "CharacterSet.h"

#if 1
#include <zlib.h>
#else
#include <zlib/zlib.h>
#endif
#if 1
#include <algorithm>
#endif

#undef tTJSBinaryStream

class XP3Stream : public IStream {

public:
	XP3Stream(CompatTJSBinaryStream *in_vfd)
	{
		ref_count = 1;
		vfd = in_vfd;
	}

	// IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject)
	{
		if (riid == IID_IUnknown || riid == IID_ISequentialStream || riid == IID_IStream)
		{
			if (ppvObject == NULL)
				return E_POINTER;
			*ppvObject = this;
			AddRef();
			return S_OK;
		}
		else
		{
			*ppvObject = 0;
			return E_NOINTERFACE;
		}
	}

	ULONG STDMETHODCALLTYPE AddRef(void)
	{
		ref_count++;
		return ref_count;
	}
	
	ULONG STDMETHODCALLTYPE Release(void)
	{
		int ret = --ref_count;
		if (ret <= 0) {
			delete this;
			ret = 0;
		}
		return ret;
	}

	// ISequentialStream
	HRESULT STDMETHODCALLTYPE Read(void *pv, ULONG cb, ULONG *pcbRead)
	{
		try
		{
			ULONG read;
			read = vfd->Read(pv, cb);
			if(pcbRead) *pcbRead = read;
		}
		catch(...)
		{
			return E_FAIL;
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Write(const void *pv, ULONG cb, ULONG *pcbWritten)
	{
		return E_NOTIMPL;
	}

	// IStream
	HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove,	DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
	{
		try
		{
			switch(dwOrigin)
			{
			case STREAM_SEEK_SET:
				if(plibNewPosition)
					(*plibNewPosition).QuadPart =
						vfd->Seek(dlibMove.QuadPart, TJS_BS_SEEK_SET);
				else
						vfd->Seek(dlibMove.QuadPart, TJS_BS_SEEK_SET);
				break;
			case STREAM_SEEK_CUR:
				if(plibNewPosition)
					(*plibNewPosition).QuadPart =
						vfd->Seek(dlibMove.QuadPart, TJS_BS_SEEK_CUR);
				else
						vfd->Seek(dlibMove.QuadPart, TJS_BS_SEEK_CUR);
				break;
			case STREAM_SEEK_END:
				if(plibNewPosition)
					(*plibNewPosition).QuadPart =
						vfd->Seek(dlibMove.QuadPart, TJS_BS_SEEK_END);
				else
						vfd->Seek(dlibMove.QuadPart, TJS_BS_SEEK_END);
				break;
			default:
				return E_FAIL;
			}
		}
		catch(...)
		{
			return E_FAIL;
		}
		return S_OK;
	}
	
	HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize)
	{
		return E_NOTIMPL;
	}
	
	HRESULT STDMETHODCALLTYPE CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE Revert(void)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
	{
		return E_NOTIMPL;
	}
	
	HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
	{
		return E_NOTIMPL;
	}
	
	HRESULT STDMETHODCALLTYPE Stat(STATSTG *pstatstg, DWORD grfStatFlag)
	{
		// This method imcompletely fills the target structure, because some
		// informations like access mode or stream name are already lost
		// at this point.

		if(pstatstg)
		{
			ZeroMemory(pstatstg, sizeof(*pstatstg));

#if 0
			// pwcsName
			// this object's storage pointer does not have a name ...
			if(!(grfStatFlag &  STATFLAG_NONAME))
			{
				// anyway returns an empty string
				LPWSTR str = (LPWSTR)CoTaskMemAlloc(sizeof(*str));
				if(str == NULL) return E_OUTOFMEMORY;
				*str = TJS_W('\0');
				pstatstg->pwcsName = str;
			}
#endif

			// type
			pstatstg->type = STGTY_STREAM;

			// cbSize
			pstatstg->cbSize.QuadPart = vfd->GetSize();

			// mtime, ctime, atime unknown

			// grfMode unknown
			pstatstg->grfMode = STGM_DIRECT | STGM_READWRITE | STGM_SHARE_DENY_WRITE ;
				// Note that this method always returns flags above, regardless of the
				// actual mode.
				// In the return value, the stream is to be indicated that the
				// stream can be written, but of cource, the Write method will fail
				// if the stream is read-only.

			// grfLockSuppoted
			pstatstg->grfLocksSupported = 0;

			// grfStatBits unknown
		}
		else
		{
			return E_INVALIDARG;
		}

		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Clone(IStream **ppstm)
	{
		return E_NOTIMPL;
	}

protected:
	/**
	 * デストラクタ
	 */
	virtual ~XP3Stream()
	{
		delete vfd;
		vfd = NULL;
	}

private:
	int ref_count;
	CompatTJSBinaryStream *vfd;
};

class XP3Storage : public iTVPStorageMedia
{

public:
	XP3Storage(tTVPXP3Archive *in_fs)
	{
		ref_count = 1;
		fs = in_fs;
		char buf[3 + (sizeof(void *) * 2) + 1];
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "%s%p", "mfi", this);
		// The hash function does not work properly with numbers, so change to letters.
		this->name = new tjs_char[sizeof(buf)];
		memset(this->name, 0, sizeof(buf) * sizeof(tjs_char));
		char *p = buf;
		tjs_char *p2 = this->name;
		while(*p)
		{
			if(*p >= '0' && *p <= '9')
				*p2 = 'g' + (*p - '0');
			else if(*p >= 'A' && *p <= 'Z')
				*p2 = (*p) | 32;
			else
				*p2 = *p;
			p++;
			p2++;
		}
	}

	virtual ~XP3Storage()
	{
		if (fs)
		{
			delete fs;
			fs = NULL;
		}
		if (this->name)
		{
			delete[] this->name;
			this->name = NULL;
		}
	}

public:
	// -----------------------------------
	// iTVPStorageMedia Intefaces
	// -----------------------------------

	virtual void TJS_INTF_METHOD AddRef()
	{
		ref_count++;
	};

	virtual void TJS_INTF_METHOD Release()
	{
		if (ref_count == 1)
		{
			delete this;
		}
		else
		{
			ref_count--;
		}
	};

	// returns media name like "file", "http" etc.
	virtual void TJS_INTF_METHOD GetName(ttstr &out_name)
	{
		if (this->name)
		{
			out_name = name;
		}
		else
		{
			out_name = TJS_W("");
		}
	}

	//	virtual ttstr TJS_INTF_METHOD IsCaseSensitive() = 0;
	// returns whether this media is case sensitive or not

	// normalize domain name according with the media's rule
	virtual void TJS_INTF_METHOD NormalizeDomainName(ttstr &name)
	{
		// normalize domain name
		// make all characters small
		tjs_char *p = name.Independ();
		while(*p)
		{
			if(*p >= TJS_W('A') && *p <= TJS_W('Z'))
				*p += TJS_W('a') - TJS_W('A');
			p++;
		}
	}

	// normalize path name according with the media's rule
	virtual void TJS_INTF_METHOD NormalizePathName(ttstr &name)
	{
		// normalize path name
		// make all characters small
		tjs_char *p = name.Independ();
		while(*p)
		{
			if(*p >= TJS_W('A') && *p <= TJS_W('Z'))
				*p += TJS_W('a') - TJS_W('A');
			p++;
		}
	}

	// check file existence
	virtual bool TJS_INTF_METHOD CheckExistentStorage(const ttstr &name)
	{
		const tjs_char *ptr = name.c_str();

		// The domain name needs to be "."
		if (!TJS_strncmp(ptr, TJS_W("./"), 2))
		{
			ptr += 2;
			ttstr fname(ptr);
			tTVPArchive::NormalizeInArchiveStorageName(fname);
			return fs->IsExistent(fname);
		}
		return false;
	}

	// open a storage and return a tTJSBinaryStream instance.
	// name does not contain in-archive storage name but
	// is normalized.
	virtual tTJSBinaryStream * TJS_INTF_METHOD Open(const ttstr & name, tjs_uint32 flags) {
		if (flags == TJS_BS_READ)
		{
			const tjs_char *ptr = name.c_str();

			// The domain name needs to be "."
			if (!TJS_strncmp(ptr, TJS_W("./"), 2))
			{
				ptr += 2;
				ttstr fname(ptr);
				tTVPArchive::NormalizeInArchiveStorageName(fname);
				CompatTJSBinaryStream *stream = fs->CreateStream(fname);
				if (stream)
				{
					IStream *streamm = new XP3Stream(stream);
					if (streamm)
					{
						tTJSBinaryStream *ret = TVPCreateBinaryStreamAdapter(streamm);
						streamm->Release();
						return ret;
					}
				}
			}
		}
		return NULL;
	}

	// list files at given place
	virtual void TJS_INTF_METHOD GetListAt(const ttstr &name, iTVPStorageLister * lister)
	{
		const tjs_char *ptr = name.c_str();

		// The domain name needs to be "."
		if (!TJS_strncmp(ptr, TJS_W("./"), 2))
		{
			ptr += 2;
			// Skip extra slashes
			while (*ptr)
			{
				if (!TJS_strncmp(ptr, TJS_W("/"), 1))
				{
					ptr += 1;
				}
				else
				{
					break;
				}
			}
			ttstr fname(ptr);
			tTVPArchive::NormalizeInArchiveStorageName(fname);
			// TODO: handle directories correctly
			// Basic logic: trim leading name
			int count = fs->GetCount();
			for (int i = 0; i < count; i += 1)
			{
				ttstr filename = fs->GetName(i);
				tTVPArchive::NormalizeInArchiveStorageName(filename);
				// Skip directory
				if (filename.StartsWith(fname))
				{
					const tjs_char *ptr2 = filename.c_str() + fname.GetLen();
					ttstr fname(ptr2);
					// Only add files directly in level
					if (!TJS_strstr(ptr2, TJS_W("/")))
					{
						lister->Add(ptr2);
					}
				}
			}
		}
		else
		{
			TVPAddLog(ttstr("Unable to search in: '") + ttstr(name) + ttstr("'"));
		}
	}

	// basically the same as above,
	// check wether given name is easily accessible from local OS filesystem.
	// if true, returns local OS native name. otherwise returns an empty string.
	virtual void TJS_INTF_METHOD GetLocallyAccessibleName(ttstr &name)
	{
		name = "";
	}

	virtual void TJS_INTF_METHOD SetArchiveExtractionFilter(tTVPXP3ArchiveExtractionFilterWithUserdata filter, void *filterdata)
	{
		fs->SetArchiveExtractionFilter(filter, filterdata);
	}

	virtual void TJS_INTF_METHOD SetArchiveExtractionPreFilter(tTVPXP3ArchiveExtractionFilterWithUserdata filter, void *filterdata)
	{
		fs->SetArchiveExtractionPreFilter(filter, filterdata);
	}

private:
	tjs_uint ref_count;
	tjs_char *name;
	tTVPXP3Archive *fs;
};

static std::vector<XP3Storage*> storage_media_vector;

class XP3Encryption
{
public:
	XP3Encryption()
	{
		char buf[3 + (sizeof(void *) * 2) + 1];
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "%s%p", "enc", this);
		// The hash function does not work properly with numbers, so change to letters.
		this->name = new tjs_char[sizeof(buf)];
		memset(this->name, 0, sizeof(buf) * sizeof(tjs_char));
		char *p = buf;
		tjs_char *p2 = this->name;
		while(*p)
		{
			if(*p >= '0' && *p <= '9')
				*p2 = 'g' + (*p - '0');
			else if(*p >= 'A' && *p <= 'Z')
				*p2 = (*p) | 32;
			else
				*p2 = *p;
			p++;
			p2++;
		}
	}

	virtual ~XP3Encryption()
	{
		if (this->name)
		{
			delete[] this->name;
			this->name = NULL;
		}
	}

	virtual void TJS_INTF_METHOD GetName(ttstr &out_name)
	{
		if (this->name)
		{
			out_name = this->name;
		}
		else
		{
			out_name = TJS_W("");
		}
	}

	virtual void TJS_INTF_METHOD Filter(tTVPXP3ExtractionFilterInfo *info)
	{
	}

	static void TVP_tTVPXP3ArchiveExtractionFilter_CONVENTION FilterExec(tTVPXP3ExtractionFilterInfo *info, void *data)
	{
		if (info->SizeOfSelf != sizeof(tTVPXP3ExtractionFilterInfo))
		{
			TVPThrowExceptionMessage(TJS_W("Incompatible tTVPXP3ExtractionFilterInfo size"));
		}
		((XP3Encryption *)data)->Filter(info);
	}

	virtual void TJS_INTF_METHOD GetArchiveExtractionFilter(tTVPXP3ArchiveExtractionFilterWithUserdata &out_filter, void * &out_data)
	{
		out_filter = FilterExec;
		out_data = this;
	}

private:
	tjs_char *name;
};

static void apply_xorpad(tjs_uint8 *data, tjs_uint32 data_length, const tjs_uint8 *xorpad, tjs_uint32 xorpad_length, tjs_uint32 xorpad_offset)
{
	for (tjs_uint32 i = 0; i < data_length; i += 1)
	{
		data[i] ^= xorpad[(i + xorpad_offset) % xorpad_length];
	}
}

class XP3SimpleXorEncryption : public XP3Encryption
{
public:
	XP3SimpleXorEncryption() : XP3Encryption()
	{
		this->xorpad = NULL;
		this->xorpad_length = 0;
	}

	virtual ~XP3SimpleXorEncryption()
	{
		this->RegisterXorPad(NULL, 0);
	}

	virtual void TJS_INTF_METHOD Filter(tTVPXP3ExtractionFilterInfo *info)
	{
		if (this->xorpad != NULL)
		{
			apply_xorpad((tjs_uint8 *)(info->Buffer), (tjs_uint32)(info->BufferSize), this->xorpad, this->xorpad_length, (tjs_uint32)(info->Offset));
		}
	}

	virtual bool TJS_INTF_METHOD RegisterXorPad(const tjs_uint8 *xorpad, tjs_uint32 xorpad_length)
	{
		if (this->xorpad != NULL)
		{
			TJSAlignedDealloc(this->xorpad);
			this->xorpad = NULL;
			this->xorpad_length = 0;
		}

		if (xorpad != NULL)
		{
			this->xorpad = (tjs_uint8 *)TJSAlignedAlloc(xorpad_length, 4);
			if (this->xorpad != NULL)
			{
				this->xorpad_length = xorpad_length;
				memcpy(this->xorpad, xorpad, xorpad_length);
				return true;
			}
		}
		else
		{
			return true;
		}
		return false;
	}
private:
	tjs_uint8 *xorpad;
	tjs_uint32 xorpad_length;
};
static XP3SimpleXorEncryption *fpd_singleton_object = NULL;

static const int FPD_HDR_SIZE = 56;

#if 0
static tjs_uint16 ReadU16BEFromMem(const tjs_uint8 *mem)
{
	tjs_uint16 ret = ((tjs_uint16)mem[0] << 8) | ((tjs_uint16)mem[1]);
	return (tjs_uint16)ret;
}

static tjs_uint32 ReadU32BEFromMem(const tjs_uint8 *mem)
{
	tjs_uint32 ret =  ((tjs_uint32)mem[0] << 24) | ((tjs_uint32)mem[1] << 16) |
		((tjs_uint32)mem[2] << 8) | ((tjs_uint32)mem[3]);
	return (tjs_uint32)ret;
}
#endif

static tjs_uint64 ReadU64BEFromMem(const tjs_uint8 *mem)
{
	tjs_uint64 ret = ((tjs_uint64)mem[0] << 56) | ((tjs_uint64)mem[1] << 48) |
		((tjs_uint64)mem[2] << 40) | ((tjs_uint64)mem[3] << 32) |
		((tjs_uint64)mem[4] << 24) | ((tjs_uint64)mem[5] << 16) |
		((tjs_uint64)mem[6] << 8) | ((tjs_uint64)mem[7]);
	return (tjs_uint64)ret;
}

class StoragesSmashFile {

public:
	static ttstr mountSmash(ttstr filename, ttstr filemapname)
	{
		{
			{
				if(!TVPIsExistentStorage(filemapname))
				{
					return TJS_W("");
				}
				std::string filemap;
				{
					tTVPStreamHolder filemapstream(filemapname);
					char *buffer;
					tjs_uint size;
					buffer = new char [ (size = static_cast<tjs_uint>(filemapstream->GetSize())) +1];
					try
					{
						filemapstream->ReadBuffer(buffer, size);
						buffer[size] = 0;
						filemap = buffer;
					}
					catch(...)
					{
						delete [] buffer;
						throw;
					}
					delete [] buffer;
				}
				tTVPXP3Archive * arc = NULL;
				try
				{
					if (TVPIsXP3Archive(filename))
					{
						arc = new tTVPXP3Archive(filename);
						if (arc)
						{
							CompatTJSBinaryStream *arcst = NULL;
							char *fcd_buffer = NULL;
							try
							{
								std::string delimiter = "::";
								size_t delim_len = delimiter.length();
								std::stringstream ss (filemap);
								std::string item;

								while (getline (ss, item, '\n'))
								{
									size_t pos_start = 0;
									size_t pos_end = 0;
									std::string token;
									std::vector<std::string> res;
									size_t ind = 0;
									ttstr inarc_filename;
									ttstr inarc_fileext;
									long long inarc_start_offset = 0;
									long long inarc_length = 0;

									while ((pos_end = item.find(delimiter, pos_start)) != std::string::npos)
									{
										token = item.substr (pos_start, pos_end - pos_start);
										pos_start = pos_end + delim_len;
										res.push_back (token);
										switch (ind)
										{
											case 0:
											{
												tjs_string token_utf16;
												TVPUtf8ToUtf16(token_utf16, token);
												inarc_filename = ttstr(token_utf16.c_str());
												break;
											}
											case 1:
											{
												tjs_string token_utf16;
												TVPUtf8ToUtf16(token_utf16, token);
												inarc_fileext = ttstr(token_utf16.c_str());
												break;
											}
											case 3:
											{
												inarc_start_offset = strtoull(token.c_str(), NULL, 10);
												break;
											}
											case 4:
											{
												inarc_length = strtoull(token.c_str(), NULL, 10);
												break;
											}
											default:
											{
												break;
											}
										}
										ind += 1;
									}

									if (inarc_filename.length() != 0)
									{
										if (inarc_fileext == TJS_W("fcd"))
										{
											if (!arcst)
											{
												arcst = TVPCreateStream(filename);
											}
											if (!fcd_buffer)
											{
												fcd_buffer = new char [32];
											}
											memset(fcd_buffer, 0, 32);
											arcst->SetPosition(inarc_start_offset);
											arcst->ReadBuffer(fcd_buffer, 32);

											if (fcd_buffer[0] == 'F' && fcd_buffer[1] == 'C' && fcd_buffer[2] == 'D')
											{
												int ogg_header_offset = 12;
												if (fcd_buffer[7] != 0)
												{
													// Has loop data
													// TODO: parse loop data and generate .sli
													ogg_header_offset = 28;
												}
												char *ogg_headerchk = &fcd_buffer[ogg_header_offset];
												if (ogg_headerchk[0] == 'O' && ogg_headerchk[1] == 'g' && ogg_headerchk[2] == 'g')
												{
													inarc_start_offset += ogg_header_offset;
													inarc_length -= ogg_header_offset;
													inarc_fileext = TJS_W("Ogg");
												}
											}
										}
										tTVPXP3Archive::tArchiveItem item;
										item.OrgSize = inarc_length;
										item.ArcSize = inarc_length;
										item.Name = inarc_filename + ttstr(TJS_W(".")) + inarc_fileext;
										tTVPArchive::NormalizeInArchiveStorageName(item.Name);
										tTVPXP3ArchiveSegment seg;
										seg.IsCompressed = false;
										seg.Start = inarc_start_offset;
										seg.Offset = 0;
										seg.OrgSize = inarc_length;
										seg.ArcSize = inarc_length;
										item.Segments.push_back(seg);
										item.FileHash = 0;
										arc->ItemVector.push_back(item);
									}
								}
								arc->Count = arc->ItemVector.size();
								if (arcst)
								{
									delete arcst;
									arcst = NULL;
								}
								if (fcd_buffer)
								{
									delete [] fcd_buffer;
									fcd_buffer = NULL;
								}

								std::stable_sort(arc->ItemVector.begin(), arc->ItemVector.end());
								XP3Storage * xp3storage = new XP3Storage(arc);
								TVPRegisterStorageMedia(xp3storage);
								storage_media_vector.push_back(xp3storage);
								ttstr xp3storage_name;
								xp3storage->GetName(xp3storage_name);
								return xp3storage_name;
							}
							catch(...)
							{
								if (arcst)
								{
									delete arcst;
									arcst = NULL;
								}
								if (fcd_buffer)
								{
									delete [] fcd_buffer;
									fcd_buffer = NULL;
								}
								if (arc)
								{
									arc->Release();
									arc = NULL;
								}
								return TJS_W("");
							}
						}
					}
				}
				catch(...)
				{
					return TJS_W("");
				}
			}
		}
		return TJS_W("");
	}

	static ttstr mountSmashFPD(ttstr filename)
	{
		{
			{
				tTVPStreamHolder filestream(filename);
				bool is_fpd = false;
#if 0
				tjs_uint32 fpd_version = 0;
#endif
				tjs_uint64 fpd_entry_count = 0;
				tjs_uint64 fpd_entry_block_size = 0;
				{
					tjs_uint8 *buffer;
					buffer = new tjs_uint8 [FPD_HDR_SIZE];
					try
					{
						filestream->ReadBuffer(buffer, FPD_HDR_SIZE);
						if (memcmp(buffer, "FPD\x00", 4) == 0)
						{
							is_fpd = true;
#if 0
							fpd_version = ReadU32BEFromMem(&buffer[4]);
#endif
							fpd_entry_count = ReadU64BEFromMem(&buffer[8]);
							fpd_entry_block_size = ReadU64BEFromMem(&buffer[16]) - FPD_HDR_SIZE;
							if (fpd_entry_count * 32 > fpd_entry_block_size)
							{
								is_fpd = false;
							}
						}
					}
					catch(...)
					{
						delete [] buffer;
						throw;
					}
					delete [] buffer;
				}
				if (is_fpd)
				{
					tjs_uint8 *buffer = NULL;
					char *path_buffer = NULL;
					buffer = new tjs_uint8 [fpd_entry_block_size];
					try
					{
						filestream->SetPosition(FPD_HDR_SIZE);
						filestream->ReadBuffer(buffer, fpd_entry_block_size);
						tTVPXP3ExtractionFilterInfo filter_info(0, (tjs_uint8 *)buffer, fpd_entry_block_size, 0);
						fpd_singleton_object->Filter(&filter_info);

						tjs_uint64 entry_info_size = fpd_entry_count * 32;

						unsigned long max_pathstr_offset = 0;

						for (tjs_uint64 i = 0; i < fpd_entry_count; i += 1)
						{
							tjs_uint64 fpd_entry_item_pathstr_offset = 0;
							fpd_entry_item_pathstr_offset = ReadU64BEFromMem(&buffer[i * 32]);
							if (fpd_entry_item_pathstr_offset > max_pathstr_offset)
							{
								max_pathstr_offset = fpd_entry_item_pathstr_offset;
							}
						}

						// Is there a better way to get the size than this?
						unsigned long destlen = (unsigned long)(max_pathstr_offset * 2);

						path_buffer = new char[destlen];
						int result = uncompress(  /* uncompress from zlib */
							(unsigned char *)path_buffer,
							&destlen, (unsigned char*)&buffer[entry_info_size],
								(unsigned long)fpd_entry_block_size);
						if (result == Z_OK && destlen >= (unsigned long)max_pathstr_offset)
						{
							tTVPXP3Archive * arc = NULL;
							try
							{
								if (TVPIsXP3Archive(filename))
								{
									arc = new tTVPXP3Archive(filename);
									if (arc)
									{
										try
										{
											for (tjs_uint64 i = 0; i < fpd_entry_count; i += 1)
											{
												tjs_uint64 fpd_entry_item_pathstr_offset = 0;
												tjs_uint64 fpd_entry_item_offset = 0;
												tjs_uint64 fpd_entry_item_size = 0;
												tjs_uint64 fpd_entry_item_uncompressed_size = 0;
												fpd_entry_item_pathstr_offset = ReadU64BEFromMem(&buffer[i * 32]);
												fpd_entry_item_offset = ReadU64BEFromMem(&buffer[(i * 32) + 8]);
												fpd_entry_item_size = ReadU64BEFromMem(&buffer[(i * 32) + 16]);
												fpd_entry_item_uncompressed_size = ReadU64BEFromMem(&buffer[(i * 32) + 24]);

												// TODO: fcd handling
												std::string item_utf8;
												item_utf8 = &path_buffer[fpd_entry_item_pathstr_offset];
												tjs_string item_utf16;
												TVPUtf8ToUtf16(item_utf16, item_utf8);
												ttstr item_ttstr;
												item_ttstr = ttstr(item_utf16.c_str());
												tTVPXP3Archive::tArchiveItem item;
												item.OrgSize = (fpd_entry_item_uncompressed_size != 0) ? fpd_entry_item_uncompressed_size : fpd_entry_item_size;
												item.ArcSize = fpd_entry_item_size;
												item.Name = item_ttstr;
												tTVPArchive::NormalizeInArchiveStorageName(item.Name);
												tTVPXP3ArchiveSegment seg;
												seg.IsCompressed = fpd_entry_item_uncompressed_size != 0;
												seg.Start = fpd_entry_item_offset;
												seg.Offset = 0;
												seg.OrgSize = (fpd_entry_item_uncompressed_size != 0) ? fpd_entry_item_uncompressed_size : fpd_entry_item_size;
												seg.ArcSize = fpd_entry_item_size;
												item.Segments.push_back(seg);
												item.FileHash = 0;
												arc->ItemVector.push_back(item);
											}
											if (path_buffer)
											{
												delete [] path_buffer;
												path_buffer = NULL;
											}
											if (buffer)
											{
												delete [] buffer;
												buffer = NULL;
											}

											arc->Count = arc->ItemVector.size();
											std::stable_sort(arc->ItemVector.begin(), arc->ItemVector.end());
											XP3Storage * xp3storage = new XP3Storage(arc);
											TVPRegisterStorageMedia(xp3storage);
											storage_media_vector.push_back(xp3storage);
											ttstr xp3storage_name;
											xp3storage->GetName(xp3storage_name);
											tTVPXP3ArchiveExtractionFilterWithUserdata this_encryptionfilter;
											void *this_encryptionfilterdata;
											fpd_singleton_object->GetArchiveExtractionFilter(this_encryptionfilter, this_encryptionfilterdata);
											xp3storage->SetArchiveExtractionPreFilter(this_encryptionfilter, this_encryptionfilterdata);
											return xp3storage_name;
										}
										catch(...)
										{
											if (path_buffer)
											{
												delete [] path_buffer;
												path_buffer = NULL;
											}
											if (buffer)
											{
												delete [] buffer;
												buffer = NULL;
											}
											if (arc)
											{
												arc->Release();
												arc = NULL;
											}
											return TJS_W("");
										}
									}
								}
							}
							catch(...)
							{
								if (arc)
								{
									arc->Release();
									arc = NULL;
								}
								return TJS_W("");
							}
						}
					}
					catch(...)
					{
						if (path_buffer)
						{
							delete [] path_buffer;
							path_buffer = NULL;
						}
						if (buffer)
						{
							delete [] buffer;
							buffer = NULL;
						}
						throw;
					}
					if (path_buffer)
					{
						delete [] path_buffer;
						path_buffer = NULL;
					}
					if (buffer)
					{
						delete [] buffer;
						buffer = NULL;
					}
				}

			}
		}
		return TJS_W("");
	}

	static bool unmountSmash(ttstr medianame)
	{
		for (auto i = storage_media_vector.begin();
			i != storage_media_vector.end(); i += 1)
		{
			ttstr this_medianame;
			(*i)->GetName(this_medianame);
			if (medianame == this_medianame)
			{
				TVPUnregisterStorageMedia(*i);
				(*i)->Release();
				storage_media_vector.erase(i);
				return true;
			}
		}

		return false;
	}

	static bool registerSmashFPDXorPad(tTJSVariant encryption_var)
	{
		if (encryption_var.Type() == tvtOctet)
		{
			const tTJSVariantOctet *oct = encryption_var.AsOctetNoAddRef();
			return fpd_singleton_object->RegisterXorPad(oct->GetData(), oct->GetLength());
		}
		return fpd_singleton_object->RegisterXorPad(NULL, 0);
	}
};

NCB_ATTACH_CLASS(StoragesSmashFile, Storages) {
	NCB_METHOD(mountSmash);
	NCB_METHOD(mountSmashFPD);
	NCB_METHOD(unmountSmash);
	NCB_METHOD(registerSmashFPDXorPad);
};


static void PreRegistCallback()
{
	fpd_singleton_object = new XP3SimpleXorEncryption();
}

static void PostUnregistCallback()
{
	for (auto i = storage_media_vector.begin();
		i != storage_media_vector.end(); i += 1)
	{
		TVPUnregisterStorageMedia(*i);
	}
	if (fpd_singleton_object != NULL)
	{
		delete fpd_singleton_object;
		fpd_singleton_object = NULL;
	}
}

NCB_PRE_REGIST_CALLBACK(PreRegistCallback);
NCB_POST_UNREGIST_CALLBACK(PostUnregistCallback);
