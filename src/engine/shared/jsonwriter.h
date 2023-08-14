/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_JSONWRITER_H
#define ENGINE_SHARED_JSONWRITER_H

#include <base/system.h>

class CJsonWriter
{
	enum
	{
		STATE_INVALID,
		STATE_OBJECT,
		STATE_ARRAY,
		STATE_ATTRIBUTE,

		MAX_DEPTH=16,
	};

	class CState
	{
	public:
		unsigned char m_Kind;
		bool m_Empty;

		CState(unsigned char Kind = STATE_INVALID)
		{
			m_Kind = Kind;
			m_Empty = true;
		};
	};

	IOHANDLE m_IO;

	CState m_aStates[MAX_DEPTH];
	int m_NumStates;
	int m_Indentation;

	bool CanWriteDatatype();
	inline void WriteInternal(const char *pStr);
	void WriteInternalEscaped(const char *pStr);
	void WriteIndent(bool EndElement);
	void PushState(unsigned char NewState);
	CState *TopState();
	unsigned char PopState();
	void CompleteDataType();

public:
	// Create a new writer object without writing anything to the file yet.
	// The file will automatically be closed by the destructor.
	CJsonWriter(IOHANDLE IO);
	~CJsonWriter();

	// The root is created by beginning the first datatype (object, array, value).
	// The writer must not be used after ending the root, which must be unique.

	// Begin writing a new object
	void BeginObject();
	// End current object
	void EndObject();

	// Begin writing a new array
	void BeginArray();
	// End current array
	void EndArray();

	// Write attribute with the given name inside the current object.
	// Names inside one object should be unique, but this is not checked here.
	// Must be used to begin write anything inside objects and only there.
	// Must be followed by a datatype for the attribute value.
	void WriteAttribute(const char *pName);

	// Methods for writing value literals
	// - As array values in arrays
	// - As attribute values after beginning an attribute inside an object
	// - As root value (only once)
	void WriteStrValue(const char *pValue);
	void WriteIntValue(int Value);
	void WriteBoolValue(bool Value);
	void WriteNullValue();
};

#endif
