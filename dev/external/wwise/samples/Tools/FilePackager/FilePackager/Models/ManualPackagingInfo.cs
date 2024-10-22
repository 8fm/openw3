﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Collections.ObjectModel;
using FilePackager.Base;

namespace FilePackager.Models
{
	public class ManualPackagingInfo : ModelBase
	{
		private uint _blockSize = 1;
        private uint _blockSizeRSX = 128;
		private ObservableCollection<Package> _packages = new ObservableCollection<Package>();

		private Guid _unassignedBanksPackageId = Guid.Empty;
		private Guid _unassignedStreamsPackageId = Guid.Empty;
        private Guid _unassignedExternalsPackageId = Guid.Empty;

        private ObservableCollection<LanguagePackageIds> _defaultLanguagePackageIds = new ObservableCollection<LanguagePackageIds>();

        public ObservableCollection<Package> Packages
		{
			get
			{
				return _packages;
			}
		}

		public uint BlockSize
		{
			get { return _blockSize; }
			set { SetValue(ref _blockSize, value, "BlockSize"); }
		}

        public uint BlockSizeRSX
        {
            get { return _blockSizeRSX; }
            set { SetValue(ref _blockSizeRSX, value, "BlockSizeRSX"); }
        }

		public Guid UnassignedBanksPackageId
		{
			get { return _unassignedBanksPackageId; }
			set { SetValue(ref _unassignedBanksPackageId, value, "UnassignedBanksPackageId"); }
		}

		public Guid UnassignedStreamsPackageId
		{
			get { return _unassignedStreamsPackageId; }
			set { SetValue(ref _unassignedStreamsPackageId, value, "UnassignedStreamsPackageId"); }
		}

        public Guid UnassignedExternalsPackageId
		{
            get { return _unassignedExternalsPackageId; }
            set { SetValue(ref _unassignedExternalsPackageId, value, "UnassignedExternalsPackageId"); }
		}

        public ObservableCollection<LanguagePackageIds> DefaultLanguagePackageIds
        {
            get
            {
                return _defaultLanguagePackageIds;
            }
        }
	}
}
