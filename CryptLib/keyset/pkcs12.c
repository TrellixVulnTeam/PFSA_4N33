/****************************************************************************
*																			*
*						  cryptlib PKCS #12 Routines						*
*						Copyright Peter Gutmann 1997-2010					*
*																			*
****************************************************************************/

#if defined( INC_ALL )
  #include "crypt.h"
  #include "asn1.h"
  #include "asn1_ext.h"
  #include "keyset.h"
  #include "pkcs12.h"
#else
  #include "crypt.h"
  #include "enc_dec/asn1.h"
  #include "enc_dec/asn1_ext.h"
  #include "keyset/keyset.h"
  #include "keyset/pkcs12.h"
#endif /* Compiler-specific includes */

#ifdef USE_PKCS12

/* OID information used to read the header of a PKCS #12 keyset */

static const CMS_CONTENT_INFO oidInfoEncryptedData = { 0, 2 };

static const OID_INFO dataOIDinfo[] = {
    { OID_CMS_DATA, CRYPT_OK },
    { NULL, 0 }, { NULL, 0 }
    };

/****************************************************************************
*																			*
*								Utility Functions							*
*																			*
****************************************************************************/

/* Sanity-check the PKCS #12 information state */

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1 ) ) \
static BOOLEAN sanityCheckPKCS12( const PKCS12_INFO *pkcs12infoPtr )
	{
	const PKCS12_OBJECT_INFO *objectInfoPtr;

	assert( isReadPtr( pkcs12infoPtr, sizeof( PKCS12_INFO ) ) );

	/* Check that the basic fields are in order.  Since all of the fields are
	   optional, either of them may not be present */
	if( pkcs12infoPtr->labelLength == 0 )
		{
		/* No label, there must be an ID present */
		if( pkcs12infoPtr->idLength < 0 || \
			pkcs12infoPtr->idLength > CRYPT_MAX_HASHSIZE )
			{
			DEBUG_PUTS(( "sanityCheckPKCS12: ID" ));
			return( FALSE );
			}
		}
	else
		{
		/* There's a label, the ID is optional */
		if( pkcs12infoPtr->labelLength <= 0 || \
			pkcs12infoPtr->labelLength > CRYPT_MAX_TEXTSIZE || \
			pkcs12infoPtr->idLength < 0 || \
			pkcs12infoPtr->idLength > CRYPT_MAX_HASHSIZE )
			{
			DEBUG_PUTS(( "sanityCheckPKCS12: Label/ID" ));
			return( FALSE );
			}
		}

	/* Check that the object-specific fields have reasonable values.  This 
	   is a general check for reasonable values that's more targeted at
	   catching inadvertent memory corruption than a strict sanity check */
	objectInfoPtr = &pkcs12infoPtr->keyInfo;
	if( objectInfoPtr->data != NULL )
		{
		if( !isShortIntegerRangeNZ( objectInfoPtr->dataSize ) || \
			objectInfoPtr->payloadOffset <= 0 || \
			objectInfoPtr->payloadOffset >= objectInfoPtr->dataSize || \
			objectInfoPtr->payloadSize <= 0 || \
			objectInfoPtr->payloadSize >= objectInfoPtr->dataSize )
			{
			DEBUG_PUTS(( "sanityCheckPKCS12: Key data/payload" ));
			return( FALSE );
			}
		}
	else
		{
		if( objectInfoPtr->dataSize != 0 || \
			objectInfoPtr->payloadOffset != 0 || \
			objectInfoPtr->payloadSize != 0 )
			{
			DEBUG_PUTS(( "sanityCheckPKCS12: Spurious key data/payload" ));
			return( FALSE );
			}
		}
	if( objectInfoPtr->keySize < 0 || \
		objectInfoPtr->keySize > CRYPT_MAX_KEYSIZE || \
		objectInfoPtr->saltSize < 0 || \
		objectInfoPtr->saltSize > CRYPT_MAX_HASHSIZE || \
		objectInfoPtr->iterations < 0 || \
		objectInfoPtr->iterations > MAX_KEYSETUP_ITERATIONS )
		{
		DEBUG_PUTS(( "sanityCheckPKCS12: Key salt/iterations" ));
		return( FALSE );
		}
	objectInfoPtr = &pkcs12infoPtr->certInfo;
	if( objectInfoPtr->data != NULL )
		{
		/* Make sure that the payload is contained within the data.  The 
		   payload may be the same as the data, so we can have a start offset
		   of 0 and a size equal to the payload size */
		if( !isShortIntegerRangeNZ( objectInfoPtr->dataSize ) || \
			objectInfoPtr->payloadOffset < 0 || \
			objectInfoPtr->payloadOffset >= objectInfoPtr->dataSize || \
			objectInfoPtr->payloadSize < 0 || \
			objectInfoPtr->payloadSize > objectInfoPtr->dataSize )
			{
			DEBUG_PUTS(( "sanityCheckPKCS12: Cert data/payload" ));
			return( FALSE );
			}
		}
	else
		{
		if( objectInfoPtr->dataSize != 0 || \
			objectInfoPtr->payloadOffset != 0 || \
			objectInfoPtr->payloadSize != 0 )
			{
			DEBUG_PUTS(( "sanityCheckPKCS12: Spurious cert data/payload" ));
			return( FALSE );
			}
		}
	if( objectInfoPtr->keySize < 0 || \
		objectInfoPtr->keySize > CRYPT_MAX_KEYSIZE || \
		objectInfoPtr->saltSize < 0 || \
		objectInfoPtr->saltSize > CRYPT_MAX_HASHSIZE || \
		objectInfoPtr->iterations < 0 || \
		objectInfoPtr->iterations > MAX_KEYSETUP_ITERATIONS )
		{
		DEBUG_PUTS(( "sanityCheckPKCS12: Cert salt/iterations" ));
		return( FALSE );
		}

	/* Check that the crypto-related fields are in order.  This is a general 
	   check for reasonable values that's more targeted at catching 
	   inadvertent memory corruption than a strict sanity check */
	if( !isShortIntegerRange( pkcs12infoPtr->macSaltSize ) || \
		!isIntegerRange( pkcs12infoPtr->macIterations ) )
		{
		DEBUG_PUTS(( "sanityCheckPKCS12: MAC info" ));
		return( FALSE );
		}
	
	return( TRUE );
	}

/* Locate a PKCS #12 object based on an ID */

#define matchID( src, srcLen, dest, destLen ) \
		( ( srcLen ) > 0 && ( srcLen ) == ( destLen ) && \
		  !memcmp( ( src ), ( dest ), ( destLen ) ) )

CHECK_RETVAL_PTR STDC_NONNULL_ARG( ( 1 ) ) \
PKCS12_INFO *pkcs12FindEntry( IN_ARRAY( noPkcs12objects ) \
									const PKCS12_INFO *pkcs12info,
							  IN_LENGTH_SHORT const int noPkcs12objects,
							  IN_KEYID const CRYPT_KEYID_TYPE keyIDtype,
							  IN_BUFFER_OPT( keyIDlength ) const void *keyID, 
							  IN_LENGTH_KEYID_Z const int keyIDlength,
							  const BOOLEAN isWildcardMatch )
	{
	int i, LOOP_ITERATOR;

	assert( isReadPtrDynamic( pkcs12info, \
							  sizeof( PKCS12_INFO ) * noPkcs12objects ) );
	assert( ( keyID == NULL && keyIDlength == 0 ) || \
			isReadPtrDynamic( keyID, keyIDlength ) );

	REQUIRES_N( isShortIntegerRangeNZ( noPkcs12objects ) );
	REQUIRES_N( keyIDtype == CRYPT_KEYID_NAME || \
				keyIDtype == CRYPT_KEYID_URI || \
				keyIDtype == CRYPT_IKEYID_KEYID );
	REQUIRES_N( ( keyID == NULL && keyIDlength == 0 ) || \
				( keyID != NULL && \
				  keyIDlength > 0 && keyIDlength < MAX_ATTRIBUTE_SIZE ) );
	REQUIRES_N( isWildcardMatch == TRUE || isWildcardMatch == FALSE );
	REQUIRES_N( ( isWildcardMatch && keyID == NULL ) || !isWildcardMatch );

	/* Try and locate the appropriate object in the PKCS #12 collection */
	LOOP_MED( i = 0, i < noPkcs12objects, i++ )
		{
		const PKCS12_INFO *pkcs12infoPtr = &pkcs12info[ i ];

		/* If there's no entry at this position, continue */
		if( pkcs12infoPtr->flags == PKCS12_FLAG_NONE )
			continue;

		ENSURES_N( sanityCheckPKCS12( pkcs12infoPtr ) );

		/* If we're doing a wildcard matches, match the first private-key 
		   entry.  This is required because PKCS #12 provides almost no 
		   useful indexing information, and works because most keysets 
		   contain only a single entry */
		if( isWildcardMatch )
			{
			if( pkcs12infoPtr->keyInfo.data == NULL )
				continue;	/* No private-key data present, continue */
			return( ( PKCS12_INFO * ) pkcs12infoPtr );
			}

		/* Check for a match based on the ID type */
		switch( keyIDtype )
			{
			case CRYPT_KEYID_NAME:
			case CRYPT_KEYID_URI:
				if( matchID( pkcs12infoPtr->label, pkcs12infoPtr->labelLength,
							 keyID, keyIDlength ) )
					return( ( PKCS12_INFO * ) pkcs12infoPtr );
				break;

			case CRYPT_IKEYID_KEYID:
				if( matchID( pkcs12infoPtr->id, pkcs12infoPtr->idLength,
							 keyID, keyIDlength ) )
					return( ( PKCS12_INFO * ) pkcs12infoPtr );
				break;

			default:
				retIntError_Null();
			}
		}
	ENSURES_N( LOOP_BOUND_OK );

	return( NULL );
	}

/* Find a free PKCS #12 entry */

CHECK_RETVAL_PTR STDC_NONNULL_ARG( ( 1 ) ) \
PKCS12_INFO *pkcs12FindFreeEntry( IN_ARRAY( noPkcs12objects ) \
									const PKCS12_INFO *pkcs12info,
								  IN_LENGTH_SHORT const int noPkcs12objects, 
								  OUT_OPT_INDEX( noPkcs12objects ) int *index )
	{
	int i, LOOP_ITERATOR;

	assert( isReadPtrDynamic( pkcs12info, \
							  sizeof( PKCS12_INFO ) * noPkcs12objects ) );
	assert( ( index == NULL ) || isWritePtr( index, sizeof( int ) ) );

	REQUIRES_N( isShortIntegerRangeNZ( noPkcs12objects ) );

	/* Clear return value */
	if( index != NULL )
		*index = CRYPT_ERROR;

	LOOP_MED( i = 0, i < noPkcs12objects, i++ )
		{
		if( pkcs12info[ i ].flags == PKCS12_FLAG_NONE )
			break;
		}
	ENSURES_N( LOOP_BOUND_OK );
	if( i >= noPkcs12objects )
		return( NULL );

	/* Remember the index value (used for enumerating PKCS #12 entries) for 
	   this entry if required */
	if( index != NULL )
		*index = i;

	return( ( PKCS12_INFO * ) &pkcs12info[ i ] );
	}

/* Free object entries */

STDC_NONNULL_ARG( ( 1 ) ) \
void pkcs12freeObjectEntry( INOUT PKCS12_OBJECT_INFO *pkcs12objectInfo )
	{
	void *dataPtr = ( void * ) pkcs12objectInfo->data;
		 /* Although the data is declared 'const' since it can't be 
		    modified, we still have to be able to zeroise it on free so 
			we override the const for this */

	assert( isWritePtr( pkcs12objectInfo, sizeof( PKCS12_OBJECT_INFO ) ) );

	zeroise( dataPtr, pkcs12objectInfo->dataSize );
	clFree( "pkcs12freeObjectEntry", dataPtr );
	zeroise( pkcs12objectInfo, sizeof( PKCS12_OBJECT_INFO ) );
	}

STDC_NONNULL_ARG( ( 1 ) ) \
void pkcs12freeEntry( INOUT PKCS12_INFO *pkcs12info )
	{
	assert( isWritePtr( pkcs12info, sizeof( PKCS12_INFO ) ) );

	if( pkcs12info->macInitialised )
		krnlSendNotifier( pkcs12info->iMacContext, IMESSAGE_DECREFCOUNT );
	if( pkcs12info->keyInfo.data != NULL )
		pkcs12freeObjectEntry( &pkcs12info->keyInfo );
	if( pkcs12info->certInfo.data != NULL )
		pkcs12freeObjectEntry( &pkcs12info->certInfo );

	zeroise( pkcs12info, sizeof( PKCS12_INFO ) );
	}

STDC_NONNULL_ARG( ( 1 ) ) \
void pkcs12Free( INOUT_ARRAY( noPkcs12objects ) PKCS12_INFO *pkcs12info, 
				 IN_RANGE( 1, MAX_PKCS12_OBJECTS ) const int noPkcs12objects )
	{
	int i, LOOP_ITERATOR;

	assert( isWritePtrDynamic( pkcs12info, \
							   sizeof( PKCS12_INFO ) * noPkcs12objects ) );

	REQUIRES_V( noPkcs12objects >= 1 && \
				noPkcs12objects <= MAX_PKCS12_OBJECTS );

	LOOP_MED( i = 0, i < noPkcs12objects, i++ )
		pkcs12freeEntry( &pkcs12info[ i ] );
	ENSURES_V( LOOP_BOUND_OK );
	zeroise( pkcs12info, sizeof( PKCS12_INFO ) * noPkcs12objects );
	}

/* Read the header of a PKCS #12 keyset */

CHECK_RETVAL STDC_NONNULL_ARG( ( 1, 2, 3 ) ) \
static int readPkcs12header( INOUT STREAM *stream, 
							 OUT_INT_Z int *endPosPtr,
							 INOUT ERROR_INFO *errorInfo )
	{
	long payloadLength DUMMY_INIT, version;
	int endPos, currentPos, status;

	assert( isWritePtr( stream, sizeof( STREAM ) ) );
	assert( isWritePtr( endPosPtr, sizeof( long ) ) );
	assert( isWritePtr( errorInfo, sizeof( ERROR_INFO ) ) );

	/* Clear return value */
	*endPosPtr = 0;

	/* Read the outer header and make sure that it's valid */
	readSequence( stream, NULL );
	status = readShortInteger( stream, &version );
	if( cryptStatusOK( status ) && version != 3 )
		status = CRYPT_ERROR_BADDATA;
	if( cryptStatusOK( status ) )
		{
		status = readCMSheader( stream, dataOIDinfo, 
								FAILSAFE_ARRAYSIZE( dataOIDinfo, OID_INFO ),
								NULL, &payloadLength, 
								READCMS_FLAG_DEFINITELENGTH_OPT );
		if( cryptStatusOK( status ) && payloadLength > MAX_INTLENGTH_SHORT )
			status = CRYPT_ERROR_BADDATA;
		}
	if( cryptStatusError( status ) )
		{
		retExt( status, 
				( status, errorInfo, 
				  "Invalid PKCS #12 keyset header" ) );
		}
	REQUIRES( isIntegerRange( payloadLength ) );
	endPos = ( int ) payloadLength;

	/* PKCS #12 files created by the Norwegian Buypass CA use constructed 
	   OCTET STRINGs with the inner, primitive OCTET STRING exactly filling 
	   the outer constructed one, in effect creating one OCTET STRING nested
	   inside the other.  To deal with this, if we hit an OCTET STRING at 
	   this point then it's the inner one so we skip it to get to the actual 
	   content */
	if( peekTag( stream ) == BER_OCTETSTRING )
		readOctetStringHole( stream, &endPos, 4, DEFAULT_TAG );

	/* If we couldn't get the length from the CMS header, try again with the 
	   next level of nested data */
	if( endPos == CRYPT_UNUSED )
		{
		int length;

		status = readSequence( stream, &length );
		if( cryptStatusOK( status ) && length == CRYPT_UNUSED )
			{
			retExt( CRYPT_ERROR_BADDATA, 
					( CRYPT_ERROR_BADDATA, errorInfo, 
					  "Can't process indefinite-length PKCS #12 "
					  "content" ) );
			}
		endPos = length;	/* int -> long, readSequence() requires an int */
		}
	else
		{
		const int startPos = stell( stream );

		/* Just skip the next level of nesting.  We don't rely on the value
		   returned from readSequence() in case it has an indefinite length,
		   since we've already got a definite length earlier */
		status = readSequence( stream, NULL );
		if( cryptStatusOK( status ) )
			endPos -= ( stell( stream ) - startPos );
		}
	if( cryptStatusError( status ) )
		{
		retExt( status, 
				( status, errorInfo, 
				  "Invalid PKCS #12 keyset inner header" ) );
		}

	/* Make sure that the length information is sensible */
	currentPos = stell( stream );
	if( endPos < 16 + MIN_OBJECT_SIZE || \
		currentPos + endPos >= MAX_INTLENGTH_SHORT )
		{
		retExt( CRYPT_ERROR_BADDATA, 
				( CRYPT_ERROR_BADDATA, errorInfo, 
				  "Invalid PKCS #12 keyset length information" ) );
		}
	*endPosPtr = currentPos + endPos;

	return( CRYPT_OK );
	}

/****************************************************************************
*																			*
*								Crypto Functions							*
*																			*
****************************************************************************/

/* Set up the parameters used to derive a password for encryption/MACing */

CHECK_RETVAL STDC_NONNULL_ARG( ( 2, 4, 5 ) ) \
static int initDeriveParams( IN_HANDLE const CRYPT_USER cryptOwner,
							 OUT_BUFFER( saltMaxLength, *saltLength ) \
								void *salt,
							 IN_LENGTH_SHORT_MIN( KEYWRAP_SALTSIZE ) \
								const int saltMaxLength,
							 OUT_LENGTH_BOUNDED_Z( saltMaxLength ) \
								int *saltLength,
							 OUT_INT_Z int *iterations )
	{
	MESSAGE_DATA msgData;
	int value, status;

	assert( isWritePtrDynamic( salt, saltMaxLength ) );
	assert( isWritePtr( saltLength, sizeof( int ) ) );
	assert( isWritePtr( iterations, sizeof( int ) ) );

	REQUIRES( cryptOwner == DEFAULTUSER_OBJECT_HANDLE || \
			  isHandleRangeValid( cryptOwner ) );
	REQUIRES( saltMaxLength >= KEYWRAP_SALTSIZE && \
			  saltMaxLength < MAX_INTLENGTH_SHORT );

	/* Clear return values */
	memset( salt, 0, min( 16, saltMaxLength ) );
	*saltLength = 0;
	*iterations = 0;

	/* Generate the salt */
	setMessageData( &msgData, salt, KEYWRAP_SALTSIZE );
	status = krnlSendMessage( SYSTEM_OBJECT_HANDLE, IMESSAGE_GETATTRIBUTE_S,
							  &msgData, CRYPT_IATTRIBUTE_RANDOM_NONCE );
	if( cryptStatusError( status ) )
		return( status );
	*saltLength = KEYWRAP_SALTSIZE;

	/* In the interests of luser-proofing we force the use of a safe minimum 
	   number of iterations */
	status = krnlSendMessage( cryptOwner, IMESSAGE_GETATTRIBUTE,
							  &value, CRYPT_OPTION_KEYING_ITERATIONS );
	if( cryptStatusError( status ) || value < MIN_KEYING_ITERATIONS )
		value = MIN_KEYING_ITERATIONS;
	*iterations = value;

	return( CRYPT_OK );
	}

/* Set up an encryption/MAC context */

CHECK_RETVAL STDC_NONNULL_ARG( ( 1, 4, 6 ) ) \
static int initContext( OUT_HANDLE_OPT CRYPT_CONTEXT *iCryptContext,
						IN_ALGO const CRYPT_ALGO_TYPE cryptAlgo,
						IN_LENGTH_KEY const int keySize,
						IN_BUFFER( passwordLength ) const void *password, 
						IN_LENGTH_TEXT const int passwordLength,
						IN_BUFFER( saltLength ) const void *salt,
						IN_LENGTH_SHORT const int saltLength,
						IN_INT const int iterations,
						const BOOLEAN isCryptContext )
	{
	CRYPT_CONTEXT iLocalCryptContext;
	MESSAGE_CREATEOBJECT_INFO createInfo;
	MECHANISM_DERIVE_INFO deriveInfo;
	MESSAGE_DATA msgData;
	BYTE key[ CRYPT_MAX_KEYSIZE + 8 ], iv[ CRYPT_MAX_IVSIZE + 8 ];
	BYTE saltData[ 1 + CRYPT_MAX_IVSIZE + 8 ];
	int ivSize DUMMY_INIT, localKeySize = keySize, status;

	assert( isWritePtr( iCryptContext, sizeof( CRYPT_CONTEXT ) ) );
	assert( isReadPtrDynamic( password, passwordLength ) );
	assert( isReadPtrDynamic( salt, saltLength ) );

	REQUIRES( ( isCryptContext && isConvAlgo( cryptAlgo ) ) || \
			  ( !isCryptContext && isMacAlgo( cryptAlgo ) ) );
	REQUIRES( keySize >= bitsToBytes( 40 ) && keySize <= CRYPT_MAX_KEYSIZE );
			  /* 40 bits is a special case for certificates encrypted with
			     RC2-40 */
	REQUIRES( passwordLength >= MIN_NAME_LENGTH && \
			  passwordLength <= CRYPT_MAX_TEXTSIZE );
	REQUIRES( saltLength >= 1 && saltLength <= CRYPT_MAX_HASHSIZE );
	REQUIRES( isIntegerRangeNZ( iterations ) );
	REQUIRES( isCryptContext == TRUE || isCryptContext == FALSE );

	/* Clear return value */
	*iCryptContext = CRYPT_ERROR;

	/* Create the encryption/MAC context and get any required parameter 
	   information.  Note that this assumes that the encryption algorithm
	   is a block cipher, which always seems to be the case */
	setMessageCreateObjectInfo( &createInfo, cryptAlgo );
	status = krnlSendMessage( SYSTEM_OBJECT_HANDLE, IMESSAGE_DEV_CREATEOBJECT,
							  &createInfo, OBJECT_TYPE_CONTEXT );
	if( cryptStatusError( status ) )
		return( status );
	iLocalCryptContext = createInfo.cryptHandle;
	if( isCryptContext )
		{
		status = krnlSendMessage( iLocalCryptContext, IMESSAGE_GETATTRIBUTE, 
								  &ivSize, CRYPT_CTXINFO_IVSIZE );
		}
	if( cryptStatusError( status ) )
		{
		krnlSendNotifier( iLocalCryptContext, IMESSAGE_DECREFCOUNT );
		return( status );
		}

	/* Since the salt also includes a diversifier as its first byte we copy 
	   it to a working buffer with room for the extra data byte */
	REQUIRES( boundsCheck( 1, saltLength, 1 + CRYPT_MAX_IVSIZE ) );
	memcpy( saltData + 1, salt, saltLength );

	/* Derive the encryption/MAC key and optional IV from the password */
	if( isCryptContext )
		saltData[ 0 ] = KEYWRAP_ID_WRAPKEY;
	else
		saltData[ 0 ] = KEYWRAP_ID_MACKEY;
	setMechanismDeriveInfo( &deriveInfo, key, keySize, password, 
							passwordLength, CRYPT_ALGO_SHA1, saltData, 
							saltLength + 1, iterations );
	status = krnlSendMessage( SYSTEM_OBJECT_HANDLE, IMESSAGE_DEV_DERIVE,
							  &deriveInfo, MECHANISM_DERIVE_PKCS12 );
	if( cryptStatusOK( status ) && isCryptContext )
		{
		saltData[ 0 ] = KEYWRAP_ID_IV;
		setMechanismDeriveInfo( &deriveInfo, iv, ivSize, password, 
								passwordLength, CRYPT_ALGO_SHA1, saltData, 
								saltLength + 1, iterations );
		status = krnlSendMessage( SYSTEM_OBJECT_HANDLE, IMESSAGE_DEV_DERIVE,
								  &deriveInfo, MECHANISM_DERIVE_PKCS12 );
		}
	clearMechanismInfo( &deriveInfo );
	if( cryptStatusError( status ) )
		{
		zeroise( key, CRYPT_MAX_KEYSIZE );
		zeroise( iv, CRYPT_MAX_IVSIZE );
		krnlSendNotifier( iLocalCryptContext, IMESSAGE_DECREFCOUNT );
		return( status );
		}

	/* We need to add special-case processing for RC2-40, which is still 
	   universally used by Windows and possibly other implementations as 
	   well.  The kernel (and pretty much everything else) won't allow keys 
	   of less then MIN_KEYSIZE bytes, to get around this we create a 
	   pseudo-key consisting of two copies of the string "PKCS#12" followed 
	   by the actual key, with a total length of 19 bytes / 152 bits.  The 
	   RC2 code checks for this special string at the start of any key that 
	   it loads and only uses the last 40 bits.  This is a horrible kludge, 
	   but RC2 is disabled by default (unless USE_PKCS12 is defined) so the 
	   only time that it'll ever be used anyway is as RC2-40 */
	if( cryptAlgo == CRYPT_ALGO_RC2 && keySize == bitsToBytes( 40 ) )
		{
		memmove( key + 14, key, bitsToBytes( 40 ) );
		memcpy( key, "PKCS#12PKCS#12", 14 );
		localKeySize = 14 + bitsToBytes( 40 );
		}

	/* Create an encryption/MAC context and load the key and IV into it */
	setMessageData( &msgData, key, localKeySize );
	status = krnlSendMessage( iLocalCryptContext, IMESSAGE_SETATTRIBUTE_S, 
							  &msgData, CRYPT_CTXINFO_KEY );
	if( cryptStatusOK( status ) && isCryptContext )
		{
		setMessageData( &msgData, iv, ivSize );
		status = krnlSendMessage( iLocalCryptContext,
								  IMESSAGE_SETATTRIBUTE_S, &msgData, 
								  CRYPT_CTXINFO_IV );
		}
	zeroise( key, CRYPT_MAX_KEYSIZE );
	zeroise( iv, CRYPT_MAX_IVSIZE );
	if( cryptStatusError( status ) )
		{
		krnlSendNotifier( iLocalCryptContext, IMESSAGE_DECREFCOUNT );
		return( status );
		}
	*iCryptContext = iLocalCryptContext;

	return( CRYPT_OK );
	}

/* Create key wrap and MAC contexts from a password */

CHECK_RETVAL STDC_NONNULL_ARG( ( 1, 3, 5 ) ) \
int createPkcs12KeyWrapContext( INOUT PKCS12_OBJECT_INFO *pkcs12objectInfo,
								IN_HANDLE const CRYPT_USER cryptOwner,
								IN_BUFFER( passwordLength ) const char *password, 
								IN_LENGTH_TEXT const int passwordLength,
								OUT_HANDLE_OPT CRYPT_CONTEXT *iCryptContext,
								const BOOLEAN initParams )
	{
	int status;

	assert( isWritePtr( pkcs12objectInfo, sizeof( PKCS12_OBJECT_INFO ) ) );
	assert( isReadPtrDynamic( password, passwordLength ) );
	assert( isWritePtr( iCryptContext, sizeof( CRYPT_CONTEXT ) ) );

	REQUIRES( cryptOwner == DEFAULTUSER_OBJECT_HANDLE || \
			  isHandleRangeValid( cryptOwner ) );
	REQUIRES( passwordLength >= MIN_NAME_LENGTH && \
			  passwordLength <= CRYPT_MAX_TEXTSIZE );
	REQUIRES( initParams == TRUE || initParams == FALSE );

	/* Clear return value */
	*iCryptContext = CRYPT_ERROR;

	/* Set up the parameters for the encryption key and IV if required.
	   The only (useful) encryption algorithm that's available is 3DES, so
	   we hardcode that in */
	if( initParams )
		{
		pkcs12objectInfo->cryptAlgo = CRYPT_ALGO_3DES;
		pkcs12objectInfo->keySize = bitsToBytes( 192 );
		status = initDeriveParams( cryptOwner, pkcs12objectInfo->salt, 
								   CRYPT_MAX_HASHSIZE, 
								   &pkcs12objectInfo->saltSize,
								   &pkcs12objectInfo->iterations );
		if( cryptStatusError( status ) )
			return( status );
		}

	/* Derive the encryption key and IV from the password */
	return( initContext( iCryptContext, pkcs12objectInfo->cryptAlgo, 
						 pkcs12objectInfo->keySize, password, 
						 passwordLength, pkcs12objectInfo->salt,
						 pkcs12objectInfo->saltSize, 
						 pkcs12objectInfo->iterations, TRUE ) );
	}

CHECK_RETVAL STDC_NONNULL_ARG( ( 1, 3, 5 ) ) \
int createPkcs12MacContext( INOUT PKCS12_INFO *pkcs12info,
							IN_HANDLE const CRYPT_USER cryptOwner,
							IN_BUFFER( passwordLength ) const char *password, 
							IN_LENGTH_TEXT const int passwordLength,
							OUT_HANDLE_OPT CRYPT_CONTEXT *iCryptContext,
							const BOOLEAN initParams )
	{
	int status;

	assert( isWritePtr( pkcs12info, sizeof( PKCS12_INFO ) ) );
	assert( isReadPtrDynamic( password, passwordLength ) );
	assert( isWritePtr( iCryptContext, sizeof( CRYPT_CONTEXT ) ) );

	REQUIRES( cryptOwner == DEFAULTUSER_OBJECT_HANDLE || \
			  isHandleRangeValid( cryptOwner ) );
	REQUIRES( passwordLength >= MIN_NAME_LENGTH && \
			  passwordLength <= CRYPT_MAX_TEXTSIZE );
	REQUIRES( initParams == TRUE || initParams == FALSE );

	/* Clear return value */
	*iCryptContext = CRYPT_ERROR;

	/* Set up the parameters used to derive the MAC key if required */
	if( initParams )
		{
		status = initDeriveParams( cryptOwner, pkcs12info->macSalt, 
								   CRYPT_MAX_HASHSIZE, 
								   &pkcs12info->macSaltSize,
								   &pkcs12info->macIterations );
		if( cryptStatusError( status ) )
			return( status );
		}

	/* Derive the MAC key from the password.  PKCS #12 currently hardcodes
	   this to HMAC-SHA1 with a 160-bit key */
	return( initContext( iCryptContext, CRYPT_ALGO_HMAC_SHA1, 
						 20, password, passwordLength, pkcs12info->macSalt,
						 pkcs12info->macSaltSize, 
						 pkcs12info->macIterations, FALSE ) );
	}

/****************************************************************************
*																			*
*								Init/Shutdown Functions						*
*																			*
****************************************************************************/

/* A PKCS #12 keyset can contain steaming mounds of keys and whatnot, so 
   when we open it we parse the contents into memory for later use */

CHECK_RETVAL STDC_NONNULL_ARG( ( 1 ) ) \
static int initFunction( INOUT KEYSET_INFO *keysetInfoPtr, 
						 STDC_UNUSED const char *name,
						 STDC_UNUSED const int nameLength,
						 IN_ENUM( CRYPT_KEYOPT ) const CRYPT_KEYOPT_TYPE options )
	{
	PKCS12_INFO *pkcs12info;
	STREAM *stream = &keysetInfoPtr->keysetFile->stream;
	int endPos DUMMY_INIT, status;

	assert( isWritePtr( keysetInfoPtr, sizeof( KEYSET_INFO ) ) );

	REQUIRES( sanityCheckKeyset( keysetInfoPtr ) );
	REQUIRES( keysetInfoPtr->type == KEYSET_FILE && \
			  keysetInfoPtr->subType == KEYSET_SUBTYPE_PKCS12 );
	REQUIRES( name == NULL && nameLength == 0 );
	REQUIRES( options == CRYPT_KEYOPT_NONE || \
			  options == CRYPT_KEYOPT_CREATE );

	/* If we're opening an existing keyset skip the outer header.  We do 
	   this before we perform any setup operations to weed out potential 
	   problem keysets */
	if( options != CRYPT_KEYOPT_CREATE )
		{
		status = readPkcs12header( stream, &endPos, KEYSET_ERRINFO );
		if( cryptStatusError( status ) )
			return( status );
		}

	/* Allocate the PKCS #12 object information */
	if( ( pkcs12info = clAlloc( "initFunction", \
								sizeof( PKCS12_INFO ) * \
								MAX_PKCS12_OBJECTS ) ) == NULL )
		return( CRYPT_ERROR_MEMORY );
	memset( pkcs12info, 0, sizeof( PKCS12_INFO ) * MAX_PKCS12_OBJECTS );
	DATAPTR_SET( keysetInfoPtr->keyData, pkcs12info );
	keysetInfoPtr->keyDataSize = sizeof( PKCS12_INFO ) * MAX_PKCS12_OBJECTS;
	keysetInfoPtr->keyDataNoObjects = MAX_PKCS12_OBJECTS;

	/* If this is a newly-created keyset, there's nothing left to do */
	if( options == CRYPT_KEYOPT_CREATE )
		return( CRYPT_OK );

	/* Read all of the keys in the keyset */
	status = pkcs12ReadKeyset( &keysetInfoPtr->keysetFile->stream, 
							   pkcs12info, MAX_PKCS12_OBJECTS, endPos, 
							   KEYSET_ERRINFO );
	if( cryptStatusError( status ) )
		{
		clFree( "initFunction", pkcs12info );
		DATAPTR_SET( keysetInfoPtr->keyData, NULL );
		keysetInfoPtr->keyDataSize = 0;
		if( options != CRYPT_KEYOPT_CREATE )
			{
			/* Reset the stream position to account for the header 
			   information that we've already read */
			sseek( stream, 0 ) ;
			}
		return( status );
		}

	ENSURES( sanityCheckKeyset( keysetInfoPtr ) );

	return( CRYPT_OK );
	}

/* Shut down the PKCS #12 state, flushing information to disk if necessary */

STDC_NONNULL_ARG( ( 1 ) ) \
static int shutdownFunction( INOUT KEYSET_INFO *keysetInfoPtr )
	{
	PKCS12_INFO *pkcs12info = DATAPTR_GET( keysetInfoPtr->keyData );
	int status = CRYPT_OK;

	assert( isWritePtr( keysetInfoPtr, sizeof( KEYSET_INFO ) ) );

	REQUIRES( sanityCheckKeyset( keysetInfoPtr ) );
	REQUIRES( keysetInfoPtr->type == KEYSET_FILE && \
			  keysetInfoPtr->subType == KEYSET_SUBTYPE_PKCS12 );
	REQUIRES( DATAPTR_ISVALID( keysetInfoPtr->keyData ) );

	/* If the contents have been changed, allocate a working I/O buffer for 
	   the duration of the flush and commit the changes to disk */
#ifdef USE_PKCS12_WRITE 
	if( TEST_FLAG( keysetInfoPtr->flags, KEYSET_FLAG_DIRTY ) )
		{
		STREAM *stream = &keysetInfoPtr->keysetFile->stream;
		BYTE buffer[ SAFEBUFFER_SIZE( STREAM_BUFSIZE ) + 8 ] STACK_ALIGN_DATA;

		REQUIRES( pkcs12info != NULL );

		sseek( stream, 0 );
		memset( buffer, 0, STREAM_BUFSIZE );
				/* Keep static analysers happy */
		safeBufferInit( SAFEBUFFER_PTR( buffer ), STREAM_BUFSIZE );
		sioctlSetString( stream, STREAM_IOCTL_IOBUFFER, 
						 SAFEBUFFER_PTR( buffer ), STREAM_BUFSIZE );
		status = pkcs12Flush( stream, pkcs12info, 
							  keysetInfoPtr->keyDataNoObjects );
		sioctlSet( stream, STREAM_IOCTL_IOBUFFER, 0 );
		if( status == OK_SPECIAL )
			{
			SET_FLAG( keysetInfoPtr->flags, KEYSET_FLAG_EMPTY );
			status = CRYPT_OK;
			}
		}
#endif /* USE_PKCS12_WRITE */

	/* Free the PKCS #12 object information */
	if( pkcs12info != NULL )
		{
		pkcs12Free( pkcs12info, MAX_PKCS12_OBJECTS );
		zeroise( pkcs12info, keysetInfoPtr->keyDataSize );
		clFree( "shutdownFunction", pkcs12info );
		DATAPTR_SET( keysetInfoPtr->keyData, NULL );
		keysetInfoPtr->keyDataSize = 0;
		}

	if( cryptStatusError( status ) )
		{
		retExt( status, 
				( status, KEYSET_ERRINFO, 
				  "Couldn't send PKCS #12 data to persistent storage" ) );
		}

	return( CRYPT_OK );
	}

/****************************************************************************
*																			*
*							Keyset Access Routines							*
*																			*
****************************************************************************/

CHECK_RETVAL STDC_NONNULL_ARG( ( 1 ) ) \
int setAccessMethodPKCS12( INOUT KEYSET_INFO *keysetInfoPtr )
	{
	int status;

	assert( isWritePtr( keysetInfoPtr, sizeof( KEYSET_INFO ) ) );

	REQUIRES( keysetInfoPtr->type == KEYSET_FILE && \
			  keysetInfoPtr->subType == KEYSET_SUBTYPE_PKCS12 );

	/* Set the access method pointers */
	FNPTR_SET( keysetInfoPtr->initFunction, initFunction );
	FNPTR_SET( keysetInfoPtr->shutdownFunction, shutdownFunction );
	status = initPKCS12get( keysetInfoPtr );
	if( cryptStatusOK( status ) )
		status = initPKCS12set( keysetInfoPtr );
	return( status );
	}
#endif /* USE_PKCS12 */
