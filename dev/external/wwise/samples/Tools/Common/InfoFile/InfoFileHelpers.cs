using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.Xml.Schema;

namespace AK.Wwise.InfoFile
{
    class InfoFileHelpers
    {
        const string INFO_SCHEMA_VERSION = "9";

        /// <summary>
        /// Load an soundbanks info file. Its data is returned in a SoundBanksInfo class.
        /// </summary>
        /// <param name="file">INFO file to be loaded</param>
        static internal AK.Wwise.InfoFile.SoundBanksInfo LoadInfoFile(string in_file)
        {
            XmlReaderSettings readerSettings = new XmlReaderSettings();
            readerSettings.ValidationType = ValidationType.Schema;
            string szSchemaFolder = System.IO.Path.GetDirectoryName(System.Windows.Forms.Application.ExecutablePath);
            string szSchemaFile = System.IO.Path.Combine(szSchemaFolder, @"..\..\..\..\Data\Schemas\SoundbanksInfo.xsd");
            readerSettings.Schemas.Add("", szSchemaFile);

			AK.Wwise.InfoFile.SoundBanksInfo data = null;

            using (XmlReader reader = XmlReader.Create(in_file, readerSettings))
            {
                // Verify the schema version.
                reader.MoveToContent();
                if (reader.GetAttribute("SchemaVersion") != INFO_SCHEMA_VERSION)
                {
                    throw new Exception("Wrong Info file schema version.");
                }

                System.Xml.Serialization.XmlSerializer serializer = new System.Xml.Serialization.XmlSerializer(typeof(AK.Wwise.InfoFile.SoundBanksInfo));
                data = (SoundBanksInfo)serializer.Deserialize(reader);
            }
            
            return data;
        }
    }
}
