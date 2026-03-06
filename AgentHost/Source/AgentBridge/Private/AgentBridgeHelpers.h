// Copyright Anthropic, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace AgentBridgeHelpers
{

/** Parse the HTTP request body as a JSON object. Returns nullptr on failure. */
inline TSharedPtr<FJsonObject> ParseJsonBody(const FHttpServerRequest& Request)
{
	FUTF8ToTCHAR WByteBuffer(reinterpret_cast<const ANSICHAR*>(Request.Body.GetData()), Request.Body.Num());
	const FString RequestBody = FString::ConstructFromPtrSize(WByteBuffer.Get(), WByteBuffer.Length());

	TSharedPtr<FJsonObject> BodyObject = MakeShareable(new FJsonObject());
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(RequestBody);

	if (!FJsonSerializer::Deserialize(JsonReader, BodyObject) || !BodyObject.IsValid())
	{
		return nullptr;
	}
	return BodyObject;
}

/** Build a JSON error response string: {"success":false,"error":"..."} */
inline FString MakeErrorResponse(const FString& ErrorMsg)
{
	FString ResponseStr;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResponseStr);
	Writer->WriteObjectStart();
	Writer->WriteValue(TEXT("success"), false);
	Writer->WriteValue(TEXT("error"), ErrorMsg);
	Writer->WriteObjectEnd();
	Writer->Close();
	return ResponseStr;
}

/** Build a JSON success response string: {"success":true, ...extra fields via callback} */
inline FString MakeSuccessResponse(TFunction<void(TSharedRef<TJsonWriter<>>&)> WriteFields = nullptr)
{
	FString ResponseStr;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResponseStr);
	Writer->WriteObjectStart();
	Writer->WriteValue(TEXT("success"), true);
	if (WriteFields)
	{
		WriteFields(Writer);
	}
	Writer->WriteObjectEnd();
	Writer->Close();
	return ResponseStr;
}

/** Send a JSON string as an HTTP response. */
inline void SendJsonResponse(const FHttpResultCallback& OnComplete, const FString& JsonStr)
{
	TUniquePtr<FHttpServerResponse> Response = FHttpServerResponse::Create(JsonStr, TEXT("application/json"));
	OnComplete(MoveTemp(Response));
}

/** Get a query parameter value from the request, or empty string if not present. */
inline FString GetQueryParam(const FHttpServerRequest& Request, const FString& Key)
{
	const FString* Value = Request.QueryParams.Find(Key);
	if (Value)
	{
		return *Value;
	}
	return FString();
}

} // namespace AgentBridgeHelpers
