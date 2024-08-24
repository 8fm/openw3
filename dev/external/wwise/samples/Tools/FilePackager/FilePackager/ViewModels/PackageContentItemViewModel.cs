using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using FilePackager.Models;
using System.Windows;
using FilePackager.Base;

namespace FilePackager.ViewModels
{
    public class PackageContentItemViewModel : ContentItemBaseViewModel
	{
        private PackageViewModel _package;

		public PackageContentItemViewModel( PackageContentItem packageContentItem, PackageViewModel package )
            : base(packageContentItem)
		{
            _package = package;

            _package.PropertyChanged += Package_PropertyChanged;
		}

        void Package_PropertyChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (e.PropertyName == "IsRSX")
            {
                OnPropertyChanged("InclusionMode");
                OnPropertyChanged("HaveInclusionMode");
            }
        }

        public override void Detach()
        {
            _package.PropertyChanged -= Package_PropertyChanged;

            base.Detach();
        }

        protected override void OnModelPropertyChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            base.OnModelPropertyChanged(sender, e);

            if (e.PropertyName == "InclusionMode")
            {
                // Invalidate the resulting items
                ProjectViewModel.Current.ManualPackagingInfo.InvalidateResultingItems();
            }
        }

        public InclusionMode InclusionMode
		{
            get 
            { 
                if( HaveInclusionMode )
                    return (_contentItemBase as PackageContentItem).InclusionMode;

                // Implicit Inclusion Mode
                if (FileType == FileType.ExternalSource)
                    return InclusionMode.External;

                if (_package.IsRSX)
                {
                    if (FileType == FileType.StreamedFile)
                        return InclusionMode.Invalid;
                    
                    return InclusionMode.StreamsRSX;
                }

                if (FileType == FileType.StreamedFileRSX)
                    return InclusionMode.Invalid;
                
                return InclusionMode.Streams;
            }
            set { SetValue(_contentItemBase, "InclusionMode", value); }
		}

        public bool HaveInclusionMode
        {
            get 
            {
                return FileType == FileType.SoundBank && _package.IsRSX == false; 
            }
        }

        public bool CanIncludeExplicitly
        {
            get
            {
                InclusionMode mode = InclusionMode;

                switch (FileType)
                {
                    case FileType.SoundBank:
                        return !_package.IsRSX && // Soundbanks are never included RSX packages
                            (mode == InclusionMode.Bank || mode == InclusionMode.BankAndStreams);
                    
                    case FileType.StreamedFile:
                        return mode == InclusionMode.BankAndStreams || mode == InclusionMode.Streams;

                    case FileType.StreamedFileRSX:
                        return mode == InclusionMode.StreamsRSX;

                    case FileType.ExternalSource:
                        return mode == InclusionMode.External;
                }
                return false;
            }
        }

        static readonly ContentItemViewModel[] _noItems = new ContentItemViewModel[0];
        public IEnumerable<ContentItemViewModel> ImplicitItems
        {
            get
            {
                if (FileType == FileType.SoundBank )
                {
                    InclusionMode mode = InclusionMode;
                    if (mode == InclusionMode.BankAndStreams || mode == InclusionMode.Streams)
                    {
                        return ContentItem.ReferencedStreamedFiles.Where(i => i.FileType == FileType.StreamedFile);
                    }
                    else if (mode == InclusionMode.StreamsRSX)
                    {
                        return ContentItem.ReferencedStreamedFiles.Where(i => i.FileType == FileType.StreamedFileRSX);
                    }
                }

                return _noItems;
            }
        }
	}
}
