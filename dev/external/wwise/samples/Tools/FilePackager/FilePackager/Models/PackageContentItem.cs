using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using FilePackager.Base;

namespace FilePackager.Models
{
    public enum InclusionMode
    {
        [ResourcesDescription("InclusionModeBankAndStreams", "FilePackager")]
        BankAndStreams,

        [ResourcesDescription("InclusionModeBank", "FilePackager")]
        Bank,

        [ResourcesDescription("InclusionModeStreams", "FilePackager")]
        Streams,

        [ResourcesDescription("InclusionModeStreamsRSX", "FilePackager")]
        [EnumValueIsVisible(false)]
        StreamsRSX,

        [ResourcesDescription("InclusionModeInvalid", "FilePackager")]
        [EnumValueIsVisible(false)]
        Invalid,

        [ResourcesDescription("InclusionModeExternal", "FilePackager")]
        [EnumValueIsVisible(false)]
        External,
    }
    
    public class PackageContentItem : ContentItemBase
	{
        private InclusionMode _inclusionMode = InclusionMode.BankAndStreams;

        public PackageContentItem( UInt64 id, string language, string fileName )
            :base(id, language,fileName)
		{
		}

        public InclusionMode InclusionMode
		{
            get { return _inclusionMode; }
            set { SetValue(ref _inclusionMode, value, "InclusionMode"); }
		}
    }
}
