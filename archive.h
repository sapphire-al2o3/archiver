
inline int CmpStr(const char *a, const char *b) {
	return std::strcmp(a, b);
}

inline int CmpStr(const wchar_t *a, const wchar_t *b) {
	return std::wcscmp(a, b);
}

template <typename CharT>
struct ArchiveSignature {
};

inline const unsigned int CharToInt(const char *str) {
	return *reinterpret_cast<const unsigned int *>(str);
}

template <>
struct ArchiveSignature<char> {
	static const unsigned int value;
};

const unsigned int ArchiveSignature<char>::value = *reinterpret_cast<const unsigned int *>("ARCA");

template <>
struct ArchiveSignature<wchar_t> {
	static const unsigned int value;
};

const unsigned int ArchiveSignature<wchar_t>::value = *reinterpret_cast<const unsigned int *>("ARCW");

template <typename CharT = char>
struct _ArchiveHeader {
	
	struct File {
		unsigned int offset;
		unsigned int size;
		unsigned int name;
	};
	
	static const int MaxPathLength = 256;

	typedef CharT type;
	typedef _ArchiveHeader<CharT> self;

	char signature[4];
	unsigned int fileCount;
	File files[1];

	static bool CheckSignature(const void *buffer) {
		return reinterpret_cast<const self *>(buffer)->CheckSignature();
	}

	bool CheckSignature() const {
		return ArchiveSignature<CharT>::value == *reinterpret_cast<const int *>(signature);
	}
	
	unsigned int GetFileCount() const {
		return fileCount;
	}
	
	void *GetFile(int index) {
		if(index < fileCount && index >= 0) {
			return reinterpret_cast<void *>(reinterpret_cast<unsigned char *>(this) + files[index].offset);
		}
		return NULL;
	}

	const void *GetFile(int index) const {
		if(index < fileCount && index >= 0) {
			return reinterpret_cast<const void *>(reinterpret_cast<const unsigned char *>(this) + files[index].offset);
		}
		return NULL;
	}
	
	void *GetFile(const CharT *fileName) {
		return GetFile(IndexOf(fileName));
	}

	const void *GetFile(const CharT *fileName) const {
		return GetFile(IndexOf(fileName));
	}
	
	size_t GetFileSize(int index) const {
		if(index >= fileCount || index < 0) {
			return 0;
		}
		
		return files[index].size;
	}
	
	size_t GetFileSize(const CharT *fileName) const {
		return GetFileSize(IndexOf(fileName));
	}
	
	const CharT *GetFileName(int index) const {
		if(index >= fileCount || index < 0) {
			return NULL;
		}

		return reinterpret_cast<const CharT *>(reinterpret_cast<const unsigned char *>(this) + files[index].name);
	}
	
	bool ExistFile(const CharT *fileName) const {
		return IndexOf(fileName) >= 0;
	}
	
	int IndexOf(const CharT *fileName) const {
		int t = 0;
		int b = fileCount - 1;
		int m = (t + b) / 2;
		
		while(t <= b) {
			m = (t + b) / 2;
			const CharT *name = reinterpret_cast<const CharT *>(reinterpret_cast<const unsigned char *>(this) + files[m].name);
			int cmp = CmpStr(name, fileName);
			if(cmp == 0) {
				return m;
			} else if(cmp < 0) {
				t = m + 1;
			} else {
				b = m - 1;
			}
		}
		return -1;
	}
};

typedef _ArchiveHeader<char> ArchiveHeader;
typedef _ArchiveHeader<wchar_t> ArchiveHeaderW;
