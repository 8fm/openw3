using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using FilePackager.Base;
using System.IO;
using FilePackager.Packaging;

namespace FilePackager.ViewModels
{
	public enum FileType
	{
		SoundBank,
		StreamedFile,
        StreamedFileRSX,    // PS3
        ExternalSource,

        Unknown,
        Mixed
	}

	public class ContentItemViewModel : ViewModelBase
	{
		AK.Wwise.InfoFile.FileDescriptorType _fileDescriptor;
        FilePackageGenerator.Context.ExternalSourceInfo _externalSourceInfo;
        private long _size = -1;
        private UInt64 _id = 0;

		public ContentItemViewModel(AK.Wwise.InfoFile.FileDescriptorType fileDescriptor)
		{
			_fileDescriptor = fileDescriptor;
            _id = _fileDescriptor.Id;
        }

        public ContentItemViewModel(FilePackageGenerator.Context.ExternalSourceInfo externalSourceInfo)
        {
            _externalSourceInfo = externalSourceInfo;
            _id = _externalSourceInfo.Id;
        }
        
        public string FileName 
		{
            get { return _fileDescriptor != null ? _fileDescriptor.ShortName : _externalSourceInfo.Name; }
		}

        public long Size
        {
            get
            {
                if (_size == -1)
                {
                    string path = string.Empty;

                    if (FileType == FileType.SoundBank)
                    {
                        path = Path.Combine(ProjectViewModel.Current.SoundBanksRoot, _fileDescriptor.Path);
                    }
                    else if (FileType == FileType.StreamedFile || FileType == FileType.StreamedFileRSX)
                    {
                        path = Path.Combine(ProjectViewModel.Current.SourceFilesRoot, _fileDescriptor.Path);
                    }
                    else if (FileType == FileType.ExternalSource)
                    {
                        path = _externalSourceInfo.Path;
                    }

                    System.IO.FileInfo fileInfo = new System.IO.FileInfo(path);
                    if (fileInfo.Exists)
                    {
                        _size = fileInfo.Length;
                    }
                    else
                    {
                        _size = 0;
                    }
                }

                return _size;
            }
        }

		public FileType FileType 
		{
			get
			{
                if (_externalSourceInfo != null)
                    return FileType.ExternalSource;

                if (_fileDescriptor.GetType() == typeof(AK.Wwise.InfoFile.SoundBank))
					return FileType.SoundBank;

				if (_fileDescriptor.GetType() != typeof(AK.Wwise.InfoFile.File))
					throw new NotSupportedException();

                if( (_fileDescriptor as AK.Wwise.InfoFile.File).RSX )
                    return FileType.StreamedFileRSX;

				return FileType.StreamedFile;
			}
		}

		public string Language 
		{
			get 
            {
                return _fileDescriptor != null ? _fileDescriptor.Language : "SFX"; 
            }	
		}

        public UInt64 Id
        {
            get { return _id; }
        }

		public string References
		{
			get
			{
                IEnumerable<ReferenceManager.PackageReference> references = ProjectViewModel.Current.ReferenceManager.GetReferences(this);

                if (references != null)
                {
                    IEnumerable<PackageViewModel> packages = references.Select(p => p.Package).OrderBy(p => p.Name);
                    return packages.ToString(", ", p => p.Name);
                }
                return "";
			}
		}

        public AK.Wwise.InfoFile.FileDescriptorType FileDescriptor
        {
            get { return _fileDescriptor; }
        }

        public FilePackageGenerator.Context.ExternalSourceInfo ExternalSourceInfo
        {
            get { return _externalSourceInfo; }
        }

		internal void OnReferencesChanged()
		{
            OnPropertyChanged("References");
		}

        static readonly IEnumerable<ContentItemViewModel> _empty = new ContentItemViewModel[0];
        public IEnumerable<ContentItemViewModel> ReferencedStreamedFiles
        {
            get
            {
                IEnumerable<ContentItemViewModel> streamedFiles = _empty;

                AK.Wwise.InfoFile.SoundBank soundBank = _fileDescriptor as AK.Wwise.InfoFile.SoundBank;
                if (soundBank != null)
                {
                    streamedFiles = soundBank.ReferencedStreamedFiles.FileCollection
                        .Cast<AK.Wwise.InfoFile.ReferencedStreamedFilesFile>()
                        .Select(file => ProjectViewModel.Current.GetContentItemLanguageSafe(file.Id, Language));
                }

                return streamedFiles;
            }
        }
    }
}
