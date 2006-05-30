/*****************************************************************
|
|    AP4 - Atom Factory
|
|    Copyright 2002-2006 Gilles Boccon-Gibod & Julien Boeuf
|
|
|    This file is part of Bento4/AP4 (MP4 Atom Processing Library).
|
|    Unless you have obtained Bento4 under a difference license,
|    this version of Bento4 is Bento4|GPL.
|    Bento4|GPL is free software; you can redistribute it and/or modify
|    it under the terms of the GNU General Public License as published by
|    the Free Software Foundation; either version 2, or (at your option)
|    any later version.
|
|    Bento4|GPL is distributed in the hope that it will be useful,
|    but WITHOUT ANY WARRANTY; without even the implied warranty of
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|    GNU General Public License for more details.
|
|    You should have received a copy of the GNU General Public License
|    along with Bento4|GPL; see the file COPYING.  If not, write to the
|    Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
|    02111-1307, USA.
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Types.h"
#include "Ap4AtomFactory.h"
#include "Ap4SampleEntry.h"
#include "Ap4IsmaCryp.h"
#include "Ap4UrlAtom.h"
#include "Ap4MoovAtom.h"
#include "Ap4MvhdAtom.h"
#include "Ap4TrakAtom.h"
#include "Ap4HdlrAtom.h"
#include "Ap4DrefAtom.h"
#include "Ap4TkhdAtom.h"
#include "Ap4MdhdAtom.h"
#include "Ap4StsdAtom.h"
#include "Ap4StscAtom.h"
#include "Ap4StcoAtom.h"
#include "Ap4StszAtom.h"
#include "Ap4EsdsAtom.h"
#include "Ap4SttsAtom.h"
#include "Ap4CttsAtom.h"
#include "Ap4StssAtom.h"
#include "Ap4FtypAtom.h"
#include "Ap4VmhdAtom.h"
#include "Ap4SmhdAtom.h"
#include "Ap4NmhdAtom.h"
#include "Ap4HmhdAtom.h"
#include "Ap4SchmAtom.h"
#include "Ap4FrmaAtom.h"
#include "Ap4TimsAtom.h"
#include "Ap4RtpAtom.h"
#include "Ap4SdpAtom.h"
#include "Ap4IkmsAtom.h"
#include "Ap4IsfmAtom.h"
#include "Ap4IsltAtom.h"
#include "Ap4TrefTypeAtom.h"
#include "Ap4MetaData.h"

/*----------------------------------------------------------------------
|   AP4_AtomFactory::~AP4_AtomFactory
+---------------------------------------------------------------------*/
AP4_AtomFactory::~AP4_AtomFactory()
{
    m_TypeHandlers.DeleteReferences();
}

/*----------------------------------------------------------------------
|   AP4_AtomFactory::AddTypeHandler
+---------------------------------------------------------------------*/
AP4_Result
AP4_AtomFactory::AddTypeHandler(TypeHandler* handler)
{
    return m_TypeHandlers.Add(handler);
}

/*----------------------------------------------------------------------
|   AP4_AtomFactory::RemoveTypeHandler
+---------------------------------------------------------------------*/
AP4_Result
AP4_AtomFactory::RemoveTypeHandler(TypeHandler* handler)
{
    return m_TypeHandlers.Remove(handler);
}

/*----------------------------------------------------------------------
|   AP4_AtomFactory::CreateAtomFromStream
+---------------------------------------------------------------------*/
AP4_Result
AP4_AtomFactory::CreateAtomFromStream(AP4_ByteStream& stream, 
                                      AP4_Atom*&      atom)
{
    AP4_Size bytes_available = 0;
    if (AP4_FAILED(stream.GetSize(bytes_available)) ||
        bytes_available == 0) {
        bytes_available = (AP4_Size)((unsigned long)(-1));
    }
    return CreateAtomFromStream(stream, bytes_available, atom);
}

/*----------------------------------------------------------------------
|   AP4_AtomFactory::CreateAtomFromStream
+---------------------------------------------------------------------*/
AP4_Result
AP4_AtomFactory::CreateAtomFromStream(AP4_ByteStream& stream, 
                                      AP4_Size&       bytes_available,
                                      AP4_Atom*&      atom)
{
    AP4_Result result;

    // NULL by default
    atom = NULL;

    // check that there are enough bytes for at least a header
    if (bytes_available < 8) return AP4_ERROR_EOS;

    // remember current stream offset
    AP4_Offset start;
    stream.Tell(start);

    // read atom size
    AP4_UI32 size;
    result = stream.ReadUI32(size);
    if (AP4_FAILED(result)) {
        stream.Seek(start);
        return result;
    }

    // read atom type
    AP4_Atom::Type type;
    result = stream.ReadUI32(type);
    if (AP4_FAILED(result)) {
        stream.Seek(start);
        return result;
    }

    // handle special size values
    if (size == 0) {
        // atom extends to end of file
        AP4_Size streamSize = 0;
        stream.GetSize(streamSize);
        if (streamSize >= start) {
            size = streamSize - start;
        }
    } else if (size == 1) {
        // 64-bit size
        if (bytes_available < 16) {
            stream.Seek(start);
            return AP4_ERROR_INVALID_FORMAT;
        }
        AP4_UI32 size_h, size_l;
        stream.ReadUI32(size_h);
        stream.ReadUI32(size_l);
        if (size_h != 0) {
            // we don't handle large 64-bit sizes yet
            stream.Seek(start);
            return AP4_ERROR_UNSUPPORTED;
        }
        size = size_l;
    }

    // check the size
    if ((size > 0 && size < 8) || size > bytes_available) {
        stream.Seek(start);
        return AP4_ERROR_INVALID_FORMAT;
    }

    // create the atom
    switch (type) {
      case AP4_ATOM_TYPE_MOOV:
        atom = AP4_MoovAtom::Create(size, stream, *this);
        break;

      case AP4_ATOM_TYPE_MVHD:
        atom = AP4_MvhdAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_TRAK:
        atom = AP4_TrakAtom::Create(size, stream, *this);
        break;

      case AP4_ATOM_TYPE_HDLR:
        atom = AP4_HdlrAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_DREF:
        atom = AP4_DrefAtom::Create(size, stream, *this);
        break;

      case AP4_ATOM_TYPE_URL:
        atom = AP4_UrlAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_TKHD:
        atom = AP4_TkhdAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_MDHD:
        atom = AP4_MdhdAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_STSD:
        atom = AP4_StsdAtom::Create(size, stream, *this);
        break;

      case AP4_ATOM_TYPE_STSC:
        atom = AP4_StscAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_STCO:
        atom = AP4_StcoAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_STSZ:
        atom = AP4_StszAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_STTS:
        atom = AP4_SttsAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_CTTS:
        atom = AP4_CttsAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_STSS:
        atom = AP4_StssAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_MP4S:
        atom = new AP4_Mp4sSampleEntry(size, stream, *this);
        break;

      case AP4_ATOM_TYPE_MP4A:
        atom = new AP4_Mp4aSampleEntry(size, stream, *this);
        break;

      case AP4_ATOM_TYPE_MP4V:
        atom = new AP4_Mp4vSampleEntry(size, stream, *this);
        break;

      case AP4_ATOM_TYPE_AVC1:
        atom = new AP4_Avc1SampleEntry(size, stream, *this);
        break;

      case AP4_ATOM_TYPE_ENCA:
        atom = new AP4_EncaSampleEntry(size, stream, *this);
        break;

      case AP4_ATOM_TYPE_ENCV:
        atom = new AP4_EncvSampleEntry(size, stream, *this);
        break;

      case AP4_ATOM_TYPE_DRMS:
        atom = new AP4_DrmsSampleEntry(size, stream, *this);
        break;

      case AP4_ATOM_TYPE_DRMI:
        atom = new AP4_DrmiSampleEntry(size, stream, *this);
        break;

      case AP4_ATOM_TYPE_ESDS:
        atom = AP4_EsdsAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_VMHD:
        atom = AP4_VmhdAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_SMHD:
        atom = AP4_SmhdAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_NMHD:
        atom = AP4_NmhdAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_HMHD:
        atom = AP4_HmhdAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_FRMA:
        atom = AP4_FrmaAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_SCHM:
        atom = AP4_SchmAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_FTYP:
        atom = AP4_FtypAtom::Create(size, stream);
        break;
          
      case AP4_ATOM_TYPE_RTP:
        if (m_Context == AP4_ATOM_TYPE_HNTI) {
            atom = AP4_RtpAtom::Create(size, stream);
        } else {
            atom = new AP4_RtpHintSampleEntry(size, stream, *this);
        }
        break;
      
      case AP4_ATOM_TYPE_TIMS:
        atom = AP4_TimsAtom::Create(size, stream);
        break;
 
      case AP4_ATOM_TYPE_SDP:
        atom = AP4_SdpAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_IKMS:
        atom = AP4_IkmsAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_ISFM:
        atom = AP4_IsfmAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_ISLT:
        atom = AP4_IsltAtom::Create(size, stream);
        break;

      case AP4_ATOM_TYPE_HINT:
        atom = AP4_TrefTypeAtom::Create(type, size, stream);
        break;

      // container atoms
      case AP4_ATOM_TYPE_TREF:
      case AP4_ATOM_TYPE_HNTI:
      case AP4_ATOM_TYPE_STBL:
      case AP4_ATOM_TYPE_MDIA:
      case AP4_ATOM_TYPE_DINF:
      case AP4_ATOM_TYPE_MINF:
      case AP4_ATOM_TYPE_SCHI:
      case AP4_ATOM_TYPE_SINF:
      case AP4_ATOM_TYPE_UDTA:
      case AP4_ATOM_TYPE_ILST:
      case AP4_ATOM_TYPE_EDTS: 
        atom = AP4_ContainerAtom::Create(type, size, false, stream, *this);
        break;

      // full container atoms
      case AP4_ATOM_TYPE_META:
        atom = AP4_ContainerAtom::Create(type, size, true, stream, *this);
        break;

      default:
        // try all the external type handlers
        {
            atom = NULL;
            AP4_List<TypeHandler>::Item* handler_item = m_TypeHandlers.FirstItem();
            while (handler_item) {
                TypeHandler* handler = handler_item->GetData();
                if (AP4_SUCCEEDED(handler->CreateAtom(type, size, stream, m_Context, atom))) {
                    break;
                }
                handler_item = handler_item->GetNext();
            }

            if (atom == NULL) {
                // no custom handlers, create a generic atom
                atom = new AP4_UnknownAtom(type, size, stream);
            }

            break;
        }
    }
    
    // if we failed to create an atom, use a generic version
    if (atom == NULL) {
        stream.Seek(start+8);
        atom = new AP4_UnknownAtom(type, size, stream);
    }

    // skip to the end of the atom
    bytes_available -= size;
    result = stream.Seek(start+size);
    if (AP4_FAILED(result)) {
        delete atom;
        atom = NULL;
    }

    return result;
}

/*----------------------------------------------------------------------
|   AP4_DefaultAtomFactory::Instance
+---------------------------------------------------------------------*/
AP4_DefaultAtomFactory AP4_DefaultAtomFactory::Instance;

/*----------------------------------------------------------------------
|   AP4_DefaultAtomFactory::Instance
+---------------------------------------------------------------------*/
AP4_DefaultAtomFactory::AP4_DefaultAtomFactory()
{
    // register built-in type handlers
    AddTypeHandler(new AP4_MetaDataAtomTypeHandler(this));
}
