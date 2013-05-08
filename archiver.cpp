#include <iostream>
#include <fstream>
#include <vector>
#include <tuple>
#include <algorithm>
#include <memory>
#include <string>
#include "archive.h"

using namespace std;

#ifdef _WIDE_
#define _MAIN_ wmain
#define _CHAR_ wchar_t
#else
#define _MAIN_ main
#define _CHAR_ char
#endif

void DispName(const char *name) {
	cout << "file name: " << name << endl;
}

void DispName(const wchar_t *name) {
	wcout << "file name: " << name << endl;
}

template <typename T>
void DispFileList(const T *arc) {
	cout << "file count: " << arc->fileCount << endl;

	for(int i = 0; i < arc->GetFileCount(); i++) {
		const T::type *name = arc->GetFileName(i);
		DispName(name);
		cout << "file size: " << arc->GetFileSize(i) << endl;
		const char *data = static_cast<const char *>(arc->GetFile(i));

		cout << data[0] << data[1] << data[2] << data[3] << endl;
	}
}

int main(int argc, _CHAR_ *argv[])
{
	typedef tuple<basic_string<_CHAR_>, char *, size_t> FileInfo;
	vector<FileInfo> files;
	
	cout << "argc " << argc << endl;

	if(argc == 1) {
		cout << "Usage: archiver output [option...] list..." << endl;
		cout << "Option: -a alignment : set alignment" << endl;
		cout << "        -w           : use wide charactor" << endl;
		return 0;
	}
	
	if(argc == 2) {
		// アーカイブ化したファイルを読み込む
		
		ifstream ifs(argv[1], ifstream::binary);
		ifs.seekg(0, ios::end);
		size_t length = ifs.tellg();
		ifs.seekg(0, ios::beg);
		
		char *buffer = new char[length];
		ifs.read(buffer, length);

		if(_ArchiveHeader<wchar_t>::CheckSignature(buffer)) {
			_ArchiveHeader<wchar_t> *arc = reinterpret_cast<_ArchiveHeader<wchar_t> *>(buffer);
			DispFileList(arc);
		} else {
			ArchiveHeader* arc = reinterpret_cast<ArchiveHeader *>(buffer);
			DispFileList(arc);
		}
		
		delete [] buffer;
	} else if(argc > 2) {

		const _CHAR_ *output = argv[1];
		int arg = 1;
		unsigned int alignment = 128;
		bool wide = false;

		for(int i = 2; i < argc; i++) {
			if(argv[i][0] == '-') {
				switch(argv[i][1]) {
				case 'a':
				case 'A':
					alignment = atoi(argv[++i]);
					break;
				case 'w':
				case 'W':
					wide = true;
					break;
				}
				arg = i + 1;
			} else {
				break;
			}
		}
		
		cout << "wide char: " << wide << endl;
		cout << "alignment: " << alignment << endl;
		
		// ファイルをアーカイブ化する
		
		for(int i = arg; i < argc; i++) {
			cout << argv[i] << endl;
			
			ifstream ifs(argv[i], ifstream::binary);
			ifs.seekg(0, ios::end);
			size_t length = ifs.tellg();
			ifs.seekg(0, ios::beg);
			
			char *buffer = new char[length];
			ifs.read(buffer, length);
			files.push_back(make_tuple(argv[i], buffer, length));
			
		}
		
		sort(files.begin(), files.end(), [](const FileInfo &x, const FileInfo &y) { return get<0>(x) < get<0>(y); });
		
		int count = files.size();
		ofstream ofs(output, ios::binary);
		char signature[4] = {'A', 'R', 'C', wide ? 'W' : 'A'};
		ofs.write(signature, sizeof(signature));
		ofs.write(reinterpret_cast<char *>(&count), sizeof(count));
		unsigned int offset = sizeof(count) + sizeof(signature);
		if(wide) {
			offset += count * sizeof(_ArchiveHeader<wchar_t>::File);
		} else {
			offset += count * sizeof(ArchiveHeader::File);
		}
		cout << offset << endl;
		
		vector<int> nameOffsets;
		int nameTableSize = 0;
		for(int i = 0; i < count; i++) {
			int size = (get<0>(files[i]).size() + 1) * (wide ? sizeof(wchar_t) : sizeof(char));
			nameTableSize += size;
			nameOffsets.push_back(size);
		}
		
		int nameOffset = offset;
		offset += nameTableSize;

		vector<int> paddings;
		unsigned int padding = 0;
		for(int i = 0; i < count; i++) {
			padding = 0;
			if(alignment > 0 && (offset & (alignment - 1)) > 0) {
				padding = ((offset + (alignment - 1)) & ~(alignment - 1)) - offset;
				cout << i << ":" << padding << ":" << offset << endl;
			}
			offset += padding;
			ofs.write(reinterpret_cast<char *>(&offset), sizeof(offset));
			ofs.write(reinterpret_cast<char *>(&get<2>(files[i])), sizeof(size_t));
			ofs.write(reinterpret_cast<char *>(&nameOffset), sizeof(nameOffset));
			offset += get<2>(files[i]);
			nameOffset += nameOffsets[i];
			paddings.push_back(padding);
		}

		for(int i = 0; i < count; i++) {
			if(wide) {
				const int length = _ArchiveHeader<wchar_t>::MaxPathLength;
				wchar_t buf[length];
				std::fill_n(buf, length, 0);
				mbstowcs(buf, get<0>(files[i]).c_str(), get<0>(files[i]).size());
				ofs.write(reinterpret_cast<char *>(buf), (get<0>(files[i]).size() + 1) * sizeof(wchar_t));
			} else {
				char buf[ArchiveHeader::MaxPathLength];
				strncpy(buf, get<0>(files[i]).c_str(), get<0>(files[i]).size());
				ofs.write(buf, (get<0>(files[i]).size() + 1) * sizeof(char));
			}
		}

		padding = 0;
		if(alignment > 0 && (offset & (alignment - 1)) > 0) {
			padding = ((offset + (alignment - 1)) & ~(alignment - 1)) - offset;
		}
		paddings.push_back(padding);

		for(int i = 0; i < count; i++) {
			for(int j = 0; j < paddings[i]; j++) {
				ofs.put(0);
			}
			ofs.write(reinterpret_cast<char *>(get<1>(files[i])), get<2>(files[i]));
		}
	}
	
	return 0;
}
