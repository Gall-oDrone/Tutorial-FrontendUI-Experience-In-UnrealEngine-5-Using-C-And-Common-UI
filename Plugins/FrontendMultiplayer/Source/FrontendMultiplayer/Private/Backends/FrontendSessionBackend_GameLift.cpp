// Diego Gallo All Rights Reserved

#include "Backends/FrontendSessionBackend_GameLift.h"
#include "FrontendMultiplayerSettings.h"
#include "FrontendMultiplayerSubsystem.h"
#include "FrontendSessionListEntryData.h"
#include "FrontendSimulatedData.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#define LOCTEXT_NAMESPACE "FrontendSessionBackend_GameLift"

namespace FrontendGameLiftCrypto
{
	static constexpr uint32 Sha256K[64] = {
		0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
		0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
		0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
		0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
		0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfc, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
		0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
		0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
		0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffa, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
	};

	FORCEINLINE uint32 RotateRight32(uint32 Value, uint32 Bits)
	{
		return (Value >> Bits) | (Value << (32u - Bits));
	}

	/** RFC 6234 SHA-256 digest over raw bytes. */
	static TArray<uint8> ComputeSha256(const uint8* Data, int64 Length)
	{
		uint32 H0 = 0x6a09e667u;
		uint32 H1 = 0xbb67ae85u;
		uint32 H2 = 0x3c6ef372u;
		uint32 H3 = 0xa54ff53au;
		uint32 H4 = 0x510e527fu;
		uint32 H5 = 0x9b05688cu;
		uint32 H6 = 0x1f83d9abu;
		uint32 H7 = 0x5be0cd19u;

		const uint64 BitLength = static_cast<uint64>(Length) * 8ull;
		const int64 ZeroPadBytes = (55 - (Length % 64) + 64) % 64;
		const int64 PaddedLength = Length + 1 + ZeroPadBytes + 8;

		TArray<uint8> Padded;
		Padded.SetNumZeroed(static_cast<int32>(PaddedLength));
		if (Length > 0 && Data != nullptr)
		{
			FMemory::Memcpy(Padded.GetData(), Data, Length);
		}
		Padded[static_cast<int32>(Length)] = 0x80u;

		for (int32 ByteIdx = 0; ByteIdx < 8; ++ByteIdx)
		{
			Padded[static_cast<int32>(PaddedLength - 1 - ByteIdx)] = static_cast<uint8>((BitLength >> (ByteIdx * 8)) & 0xffu);
		}

		uint32 W[64];
		for (int64 ChunkOffset = 0; ChunkOffset < PaddedLength; ChunkOffset += 64)
		{
			const uint8* Chunk = Padded.GetData() + ChunkOffset;
			for (int32 Index = 0; Index < 16; ++Index)
			{
				const int32 Base = Index * 4;
				W[Index] =
					(static_cast<uint32>(Chunk[Base]) << 24) |
					(static_cast<uint32>(Chunk[Base + 1]) << 16) |
					(static_cast<uint32>(Chunk[Base + 2]) << 8) |
					static_cast<uint32>(Chunk[Base + 3]);
			}
			for (int32 Index = 16; Index < 64; ++Index)
			{
				const uint32 S0 = RotateRight32(W[Index - 15], 7) ^ RotateRight32(W[Index - 15], 18) ^ (W[Index - 15] >> 3);
				const uint32 S1 = RotateRight32(W[Index - 2], 17) ^ RotateRight32(W[Index - 2], 19) ^ (W[Index - 2] >> 10);
				W[Index] = W[Index - 16] + S0 + W[Index - 7] + S1;
			}

			uint32 A = H0;
			uint32 B = H1;
			uint32 C = H2;
			uint32 D = H3;
			uint32 E = H4;
			uint32 F = H5;
			uint32 G = H6;
			uint32 H = H7;

			for (int32 Index = 0; Index < 64; ++Index)
			{
				const uint32 S1 = RotateRight32(E, 6) ^ RotateRight32(E, 11) ^ RotateRight32(E, 25);
				const uint32 Ch = (E & F) ^ ((~E) & G);
				const uint32 Temp1 = H + S1 + Ch + Sha256K[Index] + W[Index];
				const uint32 S0 = RotateRight32(A, 2) ^ RotateRight32(A, 13) ^ RotateRight32(A, 22);
				const uint32 Maj = (A & B) ^ (A & C) ^ (B & C);
				const uint32 Temp2 = S0 + Maj;

				H = G;
				G = F;
				F = E;
				E = D + Temp1;
				D = C;
				C = B;
				B = A;
				A = Temp1 + Temp2;
			}

			H0 += A;
			H1 += B;
			H2 += C;
			H3 += D;
			H4 += E;
			H5 += F;
			H6 += G;
			H7 += H;
		}

		TArray<uint8> Digest;
		Digest.SetNumUninitialized(32);
		const uint32 Words[8] = { H0, H1, H2, H3, H4, H5, H6, H7 };
		for (int32 WordIdx = 0; WordIdx < 8; ++WordIdx)
		{
			Digest[WordIdx * 4 + 0] = static_cast<uint8>((Words[WordIdx] >> 24) & 0xffu);
			Digest[WordIdx * 4 + 1] = static_cast<uint8>((Words[WordIdx] >> 16) & 0xffu);
			Digest[WordIdx * 4 + 2] = static_cast<uint8>((Words[WordIdx] >> 8) & 0xffu);
			Digest[WordIdx * 4 + 3] = static_cast<uint8>(Words[WordIdx] & 0xffu);
		}
		return Digest;
	}

	static FString BytesToLowerHex(const TArray<uint8>& Bytes)
	{
		return BytesToHex(Bytes.GetData(), Bytes.Num()).ToLower();
	}

	static FString Sha256HexUtf8(const FString& Text)
	{
		FTCHARToUTF8 Utf8(*Text);
		return BytesToLowerHex(ComputeSha256(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length()));
	}

	/** RFC 2104 HMAC-SHA256. */
	static TArray<uint8> ComputeHmacSha256(const TArray<uint8>& KeyBytes, const FString& Message)
	{
		constexpr int32 BlockSize = 64;
		TArray<uint8> NormalizedKey;
		if (KeyBytes.Num() > BlockSize)
		{
			NormalizedKey = ComputeSha256(KeyBytes.GetData(), KeyBytes.Num());
		}
		else
		{
			NormalizedKey = KeyBytes;
		}
		NormalizedKey.SetNumZeroed(BlockSize);

		TArray<uint8> OKeyPad;
		TArray<uint8> IKeyPad;
		OKeyPad.SetNumUninitialized(BlockSize);
		IKeyPad.SetNumUninitialized(BlockSize);
		for (int32 Index = 0; Index < BlockSize; ++Index)
		{
			OKeyPad[Index] = NormalizedKey[Index] ^ 0x5cu;
			IKeyPad[Index] = NormalizedKey[Index] ^ 0x36u;
		}

		FTCHARToUTF8 MessageUtf8(*Message);
		TArray<uint8> InnerInput = IKeyPad;
		InnerInput.Append(reinterpret_cast<const uint8*>(MessageUtf8.Get()), MessageUtf8.Length());
		const TArray<uint8> InnerHash = ComputeSha256(InnerInput.GetData(), InnerInput.Num());

		TArray<uint8> OuterInput = OKeyPad;
		OuterInput.Append(InnerHash);
		return ComputeSha256(OuterInput.GetData(), OuterInput.Num());
	}

	/** Signs a GameLift JSON-RPC 1.1 POST request with AWS Signature Version 4. */
	static void SignGameLiftJsonRequest(
		const TSharedRef<IHttpRequest, ESPMode::ThreadSafe>& HttpRequest,
		const FString& Host,
		const FString& Region,
		const FString& TargetAction,
		const FString& JsonPayload,
		const FString& AccessKey,
		const FString& SecretKey,
		const FString& SessionToken)
	{
		const FDateTime UtcNow = FDateTime::UtcNow();
		const FString DateStamp = UtcNow.ToString(TEXT("%Y%m%d"));
		const FString AmzDate = UtcNow.ToString(TEXT("%Y%m%dT%H%M%SZ"));
		const FString Service = TEXT("gamelift");
		const FString ContentType = TEXT("application/x-amz-json-1.1");
		const FString PayloadHash = Sha256HexUtf8(JsonPayload);

		FString CanonicalHeaders = FString::Printf(
			TEXT("content-type:%s\nhost:%s\nx-amz-date:%s\n"),
			*ContentType,
			*Host,
			*AmzDate);

		FString SignedHeaders = TEXT("content-type;host;x-amz-date");
		if (!SessionToken.IsEmpty())
		{
			CanonicalHeaders += FString::Printf(TEXT("x-amz-security-token:%s\n"), *SessionToken);
			SignedHeaders += TEXT(";x-amz-security-token");
		}
		CanonicalHeaders += FString::Printf(TEXT("x-amz-target:%s\n"), *TargetAction);
		SignedHeaders += TEXT(";x-amz-target");

		const FString CanonicalRequest = FString::Printf(
			TEXT("POST\n/\n\n%s\n%s\n%s"),
			*CanonicalHeaders,
			*SignedHeaders,
			*PayloadHash);

		const FString CredentialScope = FString::Printf(TEXT("%s/%s/%s/aws4_request"), *DateStamp, *Region, *Service);
		const FString StringToSign = FString::Printf(
			TEXT("AWS4-HMAC-SHA256\n%s\n%s\n%s"),
			*AmzDate,
			*CredentialScope,
			*Sha256HexUtf8(CanonicalRequest));

		const FString SecretSeed = TEXT("AWS4") + SecretKey;
		FTCHARToUTF8 SecretSeedUtf8(*SecretSeed);
		TArray<uint8> InitialKey;
		InitialKey.Append(reinterpret_cast<const uint8*>(SecretSeedUtf8.Get()), SecretSeedUtf8.Length());

		const TArray<uint8> KDate = ComputeHmacSha256(InitialKey, DateStamp);
		const TArray<uint8> KRegion = ComputeHmacSha256(KDate, Region);
		const TArray<uint8> KService = ComputeHmacSha256(KRegion, Service);
		const TArray<uint8> KSigning = ComputeHmacSha256(KService, TEXT("aws4_request"));
		const FString Signature = BytesToLowerHex(ComputeHmacSha256(KSigning, StringToSign));

		const FString AuthorizationHeader = FString::Printf(
			TEXT("AWS4-HMAC-SHA256 Credential=%s/%s, SignedHeaders=%s, Signature=%s"),
			*AccessKey,
			*CredentialScope,
			*SignedHeaders,
			*Signature);

		HttpRequest->SetHeader(TEXT("Content-Type"), ContentType);
		HttpRequest->SetHeader(TEXT("X-Amz-Date"), AmzDate);
		HttpRequest->SetHeader(TEXT("X-Amz-Target"), TargetAction);
		if (!SessionToken.IsEmpty())
		{
			HttpRequest->SetHeader(TEXT("X-Amz-Security-Token"), SessionToken);
		}
		HttpRequest->SetHeader(TEXT("Authorization"), AuthorizationHeader);
	}
}

namespace
{
	FString ResolveSettingWithOverrides(
		const TCHAR* CommandLineKey,
		const TCHAR* PrimaryEnvVar,
		const TCHAR* SecondaryEnvVar,
		const FString& ConfigFallback)
	{
		FString Value;
		if (CommandLineKey != nullptr && FParse::Value(FCommandLine::Get(), CommandLineKey, Value) && !Value.IsEmpty())
		{
			return Value.TrimStartAndEnd();
		}
		if (PrimaryEnvVar != nullptr)
		{
			Value = FPlatformMisc::GetEnvironmentVariable(PrimaryEnvVar).TrimStartAndEnd();
			if (!Value.IsEmpty())
			{
				return Value;
			}
		}
		if (SecondaryEnvVar != nullptr)
		{
			Value = FPlatformMisc::GetEnvironmentVariable(SecondaryEnvVar).TrimStartAndEnd();
			if (!Value.IsEmpty())
			{
				return Value;
			}
		}
		return ConfigFallback.TrimStartAndEnd();
	}
}

UFrontendGameLiftSettings::UFrontendGameLiftSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("Frontend Multiplayer GameLift");
}

FString UFrontendGameLiftSettings::ResolveAwsRegion() const
{
	const FString Region = ResolveSettingWithOverrides(
		TEXT("GameLiftRegion="),
		TEXT("GAMELIFT_REGION"),
		TEXT("AWS_REGION"),
		AwsRegion);
	return Region.IsEmpty() ? TEXT("us-east-1") : Region;
}

FString UFrontendGameLiftSettings::ResolveFleetId() const
{
	return ResolveSettingWithOverrides(
		TEXT("GameLiftFleetId="),
		TEXT("GAMELIFT_SDK_FLEET_ID"),
		TEXT("GAMELIFT_FLEET_ID"),
		FleetId);
}

FString UFrontendGameLiftSettings::ResolveAliasId() const
{
	return ResolveSettingWithOverrides(
		TEXT("GameLiftAliasId="),
		TEXT("GAMELIFT_ALIAS_ID"),
		nullptr,
		AliasId);
}

FString UFrontendGameLiftSettings::ResolveLocation() const
{
	return ResolveSettingWithOverrides(
		TEXT("GameLiftLocation="),
		TEXT("GAMELIFT_LOCATION"),
		nullptr,
		Location);
}

FString UFrontendGameLiftSettings::ResolveApiEndpointUrl() const
{
	return ResolveSettingWithOverrides(
		TEXT("GameLiftEndpoint="),
		TEXT("GAMELIFT_ENDPOINT_URL"),
		nullptr,
		ApiEndpointUrl);
}

bool UFrontendGameLiftSettings::ResolveAwsCredentials(FString& OutAccessKey, FString& OutSecretKey, FString& OutSessionToken) const
{
#if UE_BUILD_SHIPPING
	// Never embed or accept raw IAM credentials in Shipping client builds; use ApiEndpointUrl instead.
	OutAccessKey.Reset();
	OutSecretKey.Reset();
	OutSessionToken.Reset();
	return false;
#else
	OutAccessKey = ResolveSettingWithOverrides(
		TEXT("GameLiftAccessKey="),
		TEXT("GAMELIFT_ACCESS_KEY"),
		TEXT("AWS_ACCESS_KEY_ID"),
		AccessKeyId);

	OutSecretKey = ResolveSettingWithOverrides(
		TEXT("GameLiftSecretKey="),
		TEXT("GAMELIFT_SECRET_KEY"),
		TEXT("AWS_SECRET_ACCESS_KEY"),
		SecretAccessKey);

	OutSessionToken = ResolveSettingWithOverrides(
		TEXT("GameLiftSessionToken="),
		TEXT("GAMELIFT_SESSION_TOKEN"),
		TEXT("AWS_SESSION_TOKEN"),
		SessionToken);

	return !OutAccessKey.IsEmpty() && !OutSecretKey.IsEmpty();
#endif
}

FString UFrontendGameLiftSettings::ResolveServerWebSocketUrl() const
{
	return ResolveSettingWithOverrides(
		TEXT("websocketurl="),
		TEXT("GAMELIFT_SDK_WEBSOCKET_URL"),
		nullptr,
		ServerWebSocketUrl);
}

FString UFrontendGameLiftSettings::ResolveServerHostId() const
{
	return ResolveSettingWithOverrides(
		TEXT("hostid="),
		TEXT("GAMELIFT_SDK_HOST_ID"),
		nullptr,
		ServerHostId);
}

FString UFrontendGameLiftSettings::ResolveServerProcessId() const
{
	const FString Resolved = ResolveSettingWithOverrides(
		TEXT("processid="),
		TEXT("GAMELIFT_SDK_PROCESS_ID"),
		nullptr,
		ServerProcessId);
	if (!Resolved.IsEmpty())
	{
		return Resolved;
	}
	return FString::Printf(TEXT("ue-server-%u-%s"),
		FPlatformProcess::GetCurrentProcessId(),
		*FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8));
}

FString UFrontendGameLiftSettings::ResolveServerAuthToken() const
{
	return ResolveSettingWithOverrides(
		TEXT("authtoken="),
		TEXT("GAMELIFT_SDK_AUTH_TOKEN"),
		nullptr,
		ServerAuthToken);
}

//~ Begin UFrontendSessionBackend Interface

void UFrontendSessionBackend_GameLift::Initialize(UFrontendMultiplayerSubsystem* InOwningSubsystem)
{
	Super::Initialize(InOwningSubsystem);

	const UFrontendGameLiftSettings* Settings = UFrontendGameLiftSettings::Get();
	const FString Proxy = Settings->ResolveApiEndpointUrl();
	LogDebug(FString::Printf(TEXT("[gamelift] backend ready: region=%s fleet=%s alias=%s location=%s endpoint=%s"),
		*Settings->ResolveAwsRegion(),
		*Settings->ResolveFleetId(),
		*Settings->ResolveAliasId(),
		*Settings->ResolveLocation(),
		Proxy.IsEmpty() ? TEXT("direct (SigV4)") : *Proxy));

	// On a dedicated server the WebSocket is opened from OnWorldLoaded, once the match map
	// (and its listening port) is up. Connecting here as well would open a second socket
	// under a different process id and tear the first one down.
}

void UFrontendSessionBackend_GameLift::Shutdown()
{
	CancelPendingClientRequests();
	CloseServerWebSocket();
	RetiredServerWebSocket.Reset();
	FoundSessions.Empty();
	ActiveSessionDisplayName.Reset();
	ActiveSessionMaxPlayers = 0;
	ActiveGameSessionId.Reset();
	ActivePlayerSessionId.Reset();
	Super::Shutdown();
}

void UFrontendSessionBackend_GameLift::OnWorldLoaded(UWorld* LoadedWorld)
{
	StartServerWebSocketIfConfigured(LoadedWorld);
}

void UFrontendSessionBackend_GameLift::CreateSession(FName SessionName, int32 MaxPlayers)
{
	ReportOperationStarted(EFrontendSessionOperation::Creating);
	CancelPendingClientRequests();

	const UFrontendGameLiftSettings* Settings = UFrontendGameLiftSettings::Get();
	const FString FleetId = Settings->ResolveFleetId();
	const FString AliasId = Settings->ResolveAliasId();
	const FString Location = Settings->ResolveLocation();

	if (FleetId.IsEmpty() && AliasId.IsEmpty())
	{
		LogDebug(TEXT("[gamelift] CreateSession: no FleetId or AliasId configured"));
		ReportSessionCreated(false, LOCTEXT("MissingFleetOrAlias",
			"GameLift Fleet ID or Alias ID is not configured. Set it in Project Settings > Game > Frontend Multiplayer GameLift or pass -GameLiftFleetId=fleet-..."));
		return;
	}

	const FString RequestedDisplayName = SessionName.IsNone() ? TEXT("GameLift Match") : SessionName.ToString();
	const int32 ClampedMaxPlayers = FMath::Max(1, MaxPlayers);
	const FString CreatorId = BuildLocalPlayerId();

	const UFrontendMultiplayerSettings* MpSettings = UFrontendMultiplayerSettings::Get();
	const FString MapLeaf = MpSettings->HostMap.IsNull() ? FString(TEXT("L_MatchTest")) : MpSettings->HostMap.GetAssetName();

	TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
	if (!AliasId.IsEmpty())
	{
		Payload->SetStringField(TEXT("AliasId"), AliasId);
	}
	else
	{
		Payload->SetStringField(TEXT("FleetId"), FleetId);
	}
	Payload->SetStringField(TEXT("Name"), RequestedDisplayName);
	Payload->SetNumberField(TEXT("MaximumPlayerSessionCount"), ClampedMaxPlayers);
	Payload->SetStringField(TEXT("CreatorId"), CreatorId);
	if (!Location.IsEmpty())
	{
		Payload->SetStringField(TEXT("Location"), Location);
	}

	TArray<TSharedPtr<FJsonValue>> GameProperties;
	{
		TSharedRef<FJsonObject> HostProp = MakeShared<FJsonObject>();
		HostProp->SetStringField(TEXT("Key"), TEXT("HostName"));
		HostProp->SetStringField(TEXT("Value"), CreatorId);
		GameProperties.Add(MakeShared<FJsonValueObject>(HostProp));

		TSharedRef<FJsonObject> MapProp = MakeShared<FJsonObject>();
		MapProp->SetStringField(TEXT("Key"), TEXT("MapName"));
		MapProp->SetStringField(TEXT("Value"), MapLeaf);
		GameProperties.Add(MakeShared<FJsonValueObject>(MapProp));
	}
	Payload->SetArrayField(TEXT("GameProperties"), GameProperties);

	LogDebug(FString::Printf(TEXT("[gamelift] CreateGameSession '%s' (%d players)"), *RequestedDisplayName, ClampedMaxPlayers));

	TWeakObjectPtr<ThisClass> WeakThis(this);
	SendGameLiftRequest(TEXT("GameLift.CreateGameSession"), Payload,
		[WeakThis, RequestedDisplayName, ClampedMaxPlayers](bool bSuccess, TSharedPtr<FJsonObject> ResponseObject, const FText& ErrorText)
		{
			ThisClass* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			if (!bSuccess || !ResponseObject.IsValid())
			{
				Self->ReportSessionCreated(false, ErrorText);
				return;
			}

			const TSharedPtr<FJsonObject>* GameSessionObj = nullptr;
			if (!ResponseObject->TryGetObjectField(TEXT("GameSession"), GameSessionObj) || !GameSessionObj || !GameSessionObj->IsValid())
			{
				Self->ReportSessionCreated(false, LOCTEXT("MissingGameSessionField", "GameLift response did not contain a GameSession object."));
				return;
			}

			FString GameSessionId;
			FString Status;
			(*GameSessionObj)->TryGetStringField(TEXT("GameSessionId"), GameSessionId);
			(*GameSessionObj)->TryGetStringField(TEXT("Status"), Status);

			if (GameSessionId.IsEmpty())
			{
				Self->ReportSessionCreated(false, LOCTEXT("EmptyGameSessionId", "GameLift returned an empty GameSessionId."));
				return;
			}

			Self->LogDebug(FString::Printf(TEXT("[gamelift] CreateGameSession -> %s (%s)"), *GameSessionId, *Status));

			if (Status.Equals(TEXT("ACTIVE"), ESearchCase::IgnoreCase))
			{
				Self->RequestPlayerSessionForGameSession(GameSessionId, RequestedDisplayName, ClampedMaxPlayers, /*bIsJoinRequest=*/false);
				return;
			}

			// ACTIVATING: the server has not answered ActivateGameSession yet. Poll until it does.
			const float TimeoutSeconds = FMath::Max(1.f, UFrontendGameLiftSettings::Get()->SessionActivationTimeoutSeconds);
			Self->PendingActivationGameSessionId = GameSessionId;
			Self->PendingActivationDisplayName = RequestedDisplayName;
			Self->PendingActivationMaxPlayers = ClampedMaxPlayers;
			Self->ActivationPollDeadlineSeconds = FPlatformTime::Seconds() + static_cast<double>(TimeoutSeconds);
			Self->bActivationPollRequestInFlight = false;
			Self->ActivationPollTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
				FTickerDelegate::CreateUObject(Self, &ThisClass::HandleActivationPollTick), 1.0f);
		});
}

bool UFrontendSessionBackend_GameLift::HandleActivationPollTick(float /*DeltaTime*/)
{
	PollActivatingGameSession();
	// Keep ticking while a poll is pending; CancelPendingClientRequests removes the ticker.
	return !PendingActivationGameSessionId.IsEmpty();
}

void UFrontendSessionBackend_GameLift::PollActivatingGameSession()
{
	if (PendingActivationGameSessionId.IsEmpty() || bActivationPollRequestInFlight)
	{
		return;
	}

	if (FPlatformTime::Seconds() > ActivationPollDeadlineSeconds)
	{
		LogDebug(FString::Printf(TEXT("[gamelift] game session %s did not become ACTIVE in time"), *PendingActivationGameSessionId));
		CancelPendingClientRequests();
		ReportSessionCreated(false, LOCTEXT("GameSessionActivationTimeout",
			"Timed out waiting for the GameLift server to activate the game session. Is a server process connected to the fleet?"));
		return;
	}

	bActivationPollRequestInFlight = true;

	TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("GameSessionId"), PendingActivationGameSessionId);

	TWeakObjectPtr<ThisClass> WeakThis(this);
	SendGameLiftRequest(TEXT("GameLift.DescribeGameSessions"), Payload,
		[WeakThis](bool bSuccess, TSharedPtr<FJsonObject> ResponseObject, const FText& ErrorText)
		{
			ThisClass* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			Self->bActivationPollRequestInFlight = false;

			if (Self->PendingActivationGameSessionId.IsEmpty())
			{
				return; // cancelled while in flight
			}

			if (!bSuccess || !ResponseObject.IsValid())
			{
				Self->CancelPendingClientRequests();
				Self->ReportSessionCreated(false, ErrorText);
				return;
			}

			const TArray<TSharedPtr<FJsonValue>>* SessionsArray = nullptr;
			if (!ResponseObject->TryGetArrayField(TEXT("GameSessions"), SessionsArray) || !SessionsArray || SessionsArray->Num() == 0)
			{
				return;
			}

			const TSharedPtr<FJsonObject> SessionObj = (*SessionsArray)[0]->AsObject();
			if (!SessionObj.IsValid())
			{
				return;
			}

			FString Status;
			SessionObj->TryGetStringField(TEXT("Status"), Status);

			if (Status.Equals(TEXT("ACTIVE"), ESearchCase::IgnoreCase))
			{
				const FString ReadySessionId = Self->PendingActivationGameSessionId;
				const FString ReadyDisplayName = Self->PendingActivationDisplayName;
				const int32 ReadyMaxPlayers = Self->PendingActivationMaxPlayers;
				Self->CancelPendingClientRequests();
				Self->RequestPlayerSessionForGameSession(ReadySessionId, ReadyDisplayName, ReadyMaxPlayers, /*bIsJoinRequest=*/false);
			}
			else if (Status.Equals(TEXT("TERMINATED"), ESearchCase::IgnoreCase) || Status.Equals(TEXT("ERROR"), ESearchCase::IgnoreCase))
			{
				Self->CancelPendingClientRequests();
				Self->ReportSessionCreated(false, FText::Format(
					LOCTEXT("GameSessionTerminatedDuringActivation", "GameLift game session entered {0} state during activation."),
					FText::FromString(Status)));
			}
		});
}

void UFrontendSessionBackend_GameLift::RequestPlayerSessionForGameSession(const FString& GameSessionId, const FString& DisplayName, int32 MaxPlayers, bool bIsJoinRequest)
{
	TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("GameSessionId"), GameSessionId);
	Payload->SetStringField(TEXT("PlayerId"), BuildLocalPlayerId());

	TWeakObjectPtr<ThisClass> WeakThis(this);
	SendGameLiftRequest(TEXT("GameLift.CreatePlayerSession"), Payload,
		[WeakThis, GameSessionId, DisplayName, MaxPlayers, bIsJoinRequest](bool bSuccess, TSharedPtr<FJsonObject> ResponseObject, const FText& ErrorText)
		{
			ThisClass* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			auto Fail = [Self, bIsJoinRequest](const FText& Reason)
			{
				if (bIsJoinRequest)
				{
					Self->ReportSessionJoined(false, FString(), Reason);
				}
				else
				{
					Self->ReportSessionCreated(false, Reason);
				}
			};

			if (!bSuccess || !ResponseObject.IsValid())
			{
				Fail(ErrorText);
				return;
			}

			const TSharedPtr<FJsonObject>* PlayerSessionObj = nullptr;
			if (!ResponseObject->TryGetObjectField(TEXT("PlayerSession"), PlayerSessionObj) || !PlayerSessionObj || !PlayerSessionObj->IsValid())
			{
				Fail(LOCTEXT("MissingPlayerSessionField", "GameLift response did not contain a PlayerSession object."));
				return;
			}

			FString PlayerSessionId;
			FString IpAddress;
			int32 Port = 0;
			(*PlayerSessionObj)->TryGetStringField(TEXT("PlayerSessionId"), PlayerSessionId);
			(*PlayerSessionObj)->TryGetStringField(TEXT("IpAddress"), IpAddress);
			(*PlayerSessionObj)->TryGetNumberField(TEXT("Port"), Port);

			if (IpAddress.IsEmpty() || Port <= 0)
			{
				Fail(LOCTEXT("InvalidServerEndpoint", "GameLift PlayerSession did not include a valid IP address and port."));
				return;
			}

			Self->ActiveGameSessionId = GameSessionId;
			Self->ActivePlayerSessionId = PlayerSessionId;
			Self->ActiveSessionDisplayName = DisplayName;
			Self->ActiveSessionMaxPlayers = MaxPlayers;

			const FString ConnectAddress = PlayerSessionId.IsEmpty()
				? FString::Printf(TEXT("%s:%d"), *IpAddress, Port)
				: FString::Printf(TEXT("%s:%d?PlayerSessionId=%s"), *IpAddress, Port, *FGenericPlatformHttp::UrlEncode(PlayerSessionId));

			Self->LogDebug(FString::Printf(TEXT("[gamelift] CreatePlayerSession -> %s; travelling to %s"), *PlayerSessionId, *ConnectAddress));

			if (bIsJoinRequest)
			{
				Self->ReportSessionJoined(true, ConnectAddress);
			}
			else
			{
				Self->ReportSessionCreatedRemote(ConnectAddress);
			}
		});
}

void UFrontendSessionBackend_GameLift::FindSessions()
{
	ReportOperationStarted(EFrontendSessionOperation::Finding);
	CancelPendingClientRequests();
	FoundSessions.Empty();

	const UFrontendGameLiftSettings* Settings = UFrontendGameLiftSettings::Get();
	const FString FleetId = Settings->ResolveFleetId();
	const FString AliasId = Settings->ResolveAliasId();
	const FString Location = Settings->ResolveLocation();

	if (FleetId.IsEmpty() && AliasId.IsEmpty())
	{
		LogDebug(TEXT("[gamelift] FindSessions: no FleetId or AliasId configured"));
		ReportFindSessionsFailed(LOCTEXT("FindMissingFleetOrAlias", "GameLift Fleet ID or Alias ID is not configured."));
		return;
	}

	const int32 MaxResults = FMath::Clamp(UFrontendMultiplayerSettings::Get()->MaxSearchResults, 1, 100);

	TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
	if (!AliasId.IsEmpty())
	{
		Payload->SetStringField(TEXT("AliasId"), AliasId);
	}
	else
	{
		Payload->SetStringField(TEXT("FleetId"), FleetId);
	}
	if (!Location.IsEmpty())
	{
		Payload->SetStringField(TEXT("Location"), Location);
	}
	Payload->SetStringField(TEXT("StatusFilter"), TEXT("ACTIVE"));
	Payload->SetNumberField(TEXT("Limit"), MaxResults);

	LogDebug(TEXT("[gamelift] DescribeGameSessions (ACTIVE)"));

	const double RequestStartSeconds = FPlatformTime::Seconds();
	TWeakObjectPtr<ThisClass> WeakThis(this);
	SendGameLiftRequest(TEXT("GameLift.DescribeGameSessions"), Payload,
		[WeakThis, RequestStartSeconds](bool bSuccess, TSharedPtr<FJsonObject> ResponseObject, const FText& ErrorText)
		{
			ThisClass* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			if (!bSuccess || !ResponseObject.IsValid())
			{
				Self->ReportFindSessionsFailed(ErrorText);
				return;
			}

			const int32 RoundTripPingMs = FMath::Max(1, FMath::RoundToInt(static_cast<float>((FPlatformTime::Seconds() - RequestStartSeconds) * 1000.0)));
			TArray<UFrontendSessionListEntryData*> Rows;

			const TArray<TSharedPtr<FJsonValue>>* SessionsArray = nullptr;
			if (ResponseObject->TryGetArrayField(TEXT("GameSessions"), SessionsArray) && SessionsArray)
			{
				for (const TSharedPtr<FJsonValue>& Value : *SessionsArray)
				{
					const TSharedPtr<FJsonObject> SessionObj = Value.IsValid() ? Value->AsObject() : nullptr;
					if (!SessionObj.IsValid())
					{
						continue;
					}

					FFrontendGameLiftSessionSummary Summary;
					SessionObj->TryGetStringField(TEXT("GameSessionId"), Summary.GameSessionId);
					SessionObj->TryGetStringField(TEXT("Name"), Summary.DisplayName);
					SessionObj->TryGetStringField(TEXT("CreatorId"), Summary.HostName);
					SessionObj->TryGetStringField(TEXT("IpAddress"), Summary.IpAddress);
					SessionObj->TryGetNumberField(TEXT("Port"), Summary.Port);
					SessionObj->TryGetNumberField(TEXT("CurrentPlayerSessionCount"), Summary.CurrentPlayers);
					SessionObj->TryGetNumberField(TEXT("MaximumPlayerSessionCount"), Summary.MaxPlayers);

					const TArray<TSharedPtr<FJsonValue>>* PropsArray = nullptr;
					if (SessionObj->TryGetArrayField(TEXT("GameProperties"), PropsArray) && PropsArray)
					{
						for (const TSharedPtr<FJsonValue>& PropVal : *PropsArray)
						{
							const TSharedPtr<FJsonObject> PropObj = PropVal.IsValid() ? PropVal->AsObject() : nullptr;
							if (!PropObj.IsValid())
							{
								continue;
							}
							FString Key;
							FString PropValue;
							PropObj->TryGetStringField(TEXT("Key"), Key);
							PropObj->TryGetStringField(TEXT("Value"), PropValue);
							if (Key.Equals(TEXT("HostName"), ESearchCase::IgnoreCase) && !PropValue.IsEmpty())
							{
								Summary.HostName = PropValue;
							}
							else if (Key.Equals(TEXT("MapName"), ESearchCase::IgnoreCase) && !PropValue.IsEmpty())
							{
								Summary.MapName = PropValue;
							}
						}
					}

					if (Summary.DisplayName.IsEmpty())
					{
						Summary.DisplayName = Summary.GameSessionId;
					}
					if (Summary.HostName.IsEmpty())
					{
						Summary.HostName = TEXT("GameLift Server");
					}
					if (Summary.MapName.IsEmpty())
					{
						Summary.MapName = TEXT("L_MatchTest");
					}

					Self->FoundSessions.Add(Summary);

					UFrontendSessionListEntryData* Row = NewObject<UFrontendSessionListEntryData>(Self);
					Row->SessionDisplayName = Summary.DisplayName;
					Row->HostName = Summary.HostName;
					Row->MapName = Summary.MapName;
					Row->CurrentPlayers = Summary.CurrentPlayers;
					Row->MaxPlayers = Summary.MaxPlayers;
					Row->PingInMs = RoundTripPingMs;
					Rows.Add(Row);
				}
			}

			Self->LogDebug(FString::Printf(TEXT("[gamelift] DescribeGameSessions -> %d active session(s)"), Rows.Num()));
			Self->ReportSessionsFound(Rows);
		});
}

void UFrontendSessionBackend_GameLift::JoinSession(int32 SessionIndex)
{
	ReportOperationStarted(EFrontendSessionOperation::Joining);

	if (!FoundSessions.IsValidIndex(SessionIndex))
	{
		ReportSessionJoined(false, FString(), LOCTEXT("InvalidJoinIndex", "That server is no longer in the list. Refresh and try again."));
		return;
	}

	const FFrontendGameLiftSessionSummary Selected = FoundSessions[SessionIndex];
	LogDebug(FString::Printf(TEXT("[gamelift] JoinSession index %d -> %s"), SessionIndex, *Selected.GameSessionId));

	if (Selected.MaxPlayers > 0 && Selected.CurrentPlayers >= Selected.MaxPlayers)
	{
		ReportSessionJoined(false, FString(), LOCTEXT("SessionFull", "This GameLift game session is already full."));
		return;
	}

	CancelPendingClientRequests();
	RequestPlayerSessionForGameSession(Selected.GameSessionId, Selected.DisplayName, Selected.MaxPlayers, /*bIsJoinRequest=*/true);
}

void UFrontendSessionBackend_GameLift::LeaveSession()
{
	// The server frees the slot via RemovePlayerSession when the connection drops.
	LogDebug(TEXT("[gamelift] LeaveSession"));
	CancelPendingClientRequests();
	ActiveSessionDisplayName.Reset();
	ActiveSessionMaxPlayers = 0;
	ActiveGameSessionId.Reset();
	ActivePlayerSessionId.Reset();
	ReportSessionLeft();
}

void UFrontendSessionBackend_GameLift::FetchLeaderboard(ELeaderboardCategory Category)
{
	// GameLift places and hosts sessions; it has no leaderboard service. Same policy as FixedAddress.
	if (UFrontendMultiplayerSettings::Get()->bSimulateLeaderboardWhenUnsupported)
	{
		LogDebug(FString::Printf(TEXT("[gamelift] FetchLeaderboard(%s): no leaderboard service, returning simulated rows"), *UEnum::GetDisplayValueAsText(Category).ToString()));
		ReportLeaderboard(Category, FFrontendSimulatedData::MakeLeaderboard(this, Category));
	}
	else
	{
		LogDebug(FString::Printf(TEXT("[gamelift] FetchLeaderboard(%s): no leaderboard service"), *UEnum::GetDisplayValueAsText(Category).ToString()));
		ReportLeaderboard(Category, TArray<UFrontendLeaderboardEntryData*>());
	}
}

FString UFrontendSessionBackend_GameLift::GetBackendDisplayName() const
{
	const UFrontendGameLiftSettings* Settings = UFrontendGameLiftSettings::Get();
	const FString Region = Settings->ResolveAwsRegion();
	const FString Location = Settings->ResolveLocation();
	return Location.IsEmpty()
		? FString::Printf(TEXT("GameLift (%s)"), *Region)
		: FString::Printf(TEXT("GameLift (%s / %s)"), *Region, *Location);
}

bool UFrontendSessionBackend_GameLift::GetActiveSessionInfo(FString& OutDisplayName, int32& OutMaxPlayers) const
{
	if (ActiveSessionDisplayName.IsEmpty() || ActiveSessionMaxPlayers <= 0)
	{
		return false;
	}
	OutDisplayName = ActiveSessionDisplayName;
	OutMaxPlayers = ActiveSessionMaxPlayers;
	return true;
}

bool UFrontendSessionBackend_GameLift::AcceptPlayerSession(const FString& PlayerSessionId, FString& OutErrorMessage)
{
	if (!ServerWebSocket.IsValid() || !ServerWebSocket->IsConnected() || !bServerProcessReady)
	{
		// Not under GameLift control (e.g. a plain LAN dedicated server): accept everyone.
		return true;
	}

	if (PlayerSessionId.IsEmpty())
	{
		if (UFrontendGameLiftSettings::Get()->bRequirePlayerSessionIdOnServer && !ServerGameSessionId.IsEmpty())
		{
			OutErrorMessage = TEXT("GameLift server requires a valid ?PlayerSessionId= option to join.");
			return false;
		}
		return true;
	}

	if (ServerGameSessionId.IsEmpty())
	{
		OutErrorMessage = TEXT("GameLift server has not activated a game session yet.");
		return false;
	}

	TSharedRef<FJsonObject> Message = MakeShared<FJsonObject>();
	Message->SetStringField(TEXT("Action"), TEXT("AcceptPlayerSession"));
	Message->SetStringField(TEXT("RequestId"), FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower));
	Message->SetStringField(TEXT("GameSessionId"), ServerGameSessionId);
	Message->SetStringField(TEXT("PlayerSessionId"), PlayerSessionId);
	SendServerWebSocketMessage(Message);

	// Someone came (back) in during the empty-session grace period: keep the process.
	CancelPendingServerExit();

	LogDebug(FString::Printf(TEXT("[gamelift] AcceptPlayerSession %s"), *PlayerSessionId));
	return true;
}

void UFrontendSessionBackend_GameLift::RemovePlayerSession(const FString& PlayerSessionId)
{
	if (!ServerWebSocket.IsValid() || !ServerWebSocket->IsConnected() || ServerGameSessionId.IsEmpty() || PlayerSessionId.IsEmpty())
	{
		return;
	}

	TSharedRef<FJsonObject> Message = MakeShared<FJsonObject>();
	Message->SetStringField(TEXT("Action"), TEXT("RemovePlayerSession"));
	Message->SetStringField(TEXT("RequestId"), FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower));
	Message->SetStringField(TEXT("GameSessionId"), ServerGameSessionId);
	Message->SetStringField(TEXT("PlayerSessionId"), PlayerSessionId);
	SendServerWebSocketMessage(Message);

	LogDebug(FString::Printf(TEXT("[gamelift] RemovePlayerSession %s"), *PlayerSessionId));
}

bool UFrontendSessionBackend_GameLift::HandleServerSessionEmpty()
{
	const UFrontendGameLiftSettings* Settings = UFrontendGameLiftSettings::Get();
	if (!Settings->bExitProcessWhenSessionEmpty
		|| !ServerWebSocket.IsValid() || !ServerWebSocket->IsConnected() || !bServerProcessReady
		|| ServerGameSessionId.IsEmpty())
	{
		// No GameLift game session attached: nothing to recycle, let the game mode reset the lobby.
		return false;
	}

	if (ServerSessionEmptyExitTickerHandle.IsValid())
	{
		return true; // already counting down
	}

	const float Delay = FMath::Max(0.f, Settings->SessionEmptyExitDelaySeconds);
	LogDebug(FString::Printf(TEXT("[gamelift] last player left game session %s; ending it and exiting in %.0fs so the fleet can recycle this process"),
		*ServerGameSessionId, Delay));
	ServerSessionEmptyExitTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &ThisClass::HandleSessionEmptyExitTick), Delay);
	return true;
}

bool UFrontendSessionBackend_GameLift::HandleSessionEmptyExitTick(float /*DeltaTime*/)
{
	ServerSessionEmptyExitTickerHandle.Reset();
	TerminateServerProcessAndExit(TEXT("game session empty"));
	return false; // one-shot
}

void UFrontendSessionBackend_GameLift::CancelPendingServerExit()
{
	if (ServerSessionEmptyExitTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ServerSessionEmptyExitTickerHandle);
		ServerSessionEmptyExitTickerHandle.Reset();
		LogDebug(TEXT("[gamelift] player joined during the empty-session grace period; keeping the process"));
	}
}

void UFrontendSessionBackend_GameLift::TerminateServerProcessAndExit(const TCHAR* Reason)
{
	LogDebug(FString::Printf(TEXT("[gamelift] TerminateServerProcess (%s) -> exiting"), Reason));

	TSharedRef<FJsonObject> TerminateMsg = MakeShared<FJsonObject>();
	TerminateMsg->SetStringField(TEXT("Action"), TEXT("TerminateServerProcess"));
	TerminateMsg->SetStringField(TEXT("RequestId"), FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower));
	SendServerWebSocketMessage(TerminateMsg);
	ServerGameSessionId.Reset();

	// Give the frame a moment to leave the socket before tearing it down; GameLift would
	// also notice the disconnect on its own, this just keeps the fleet events clean.
	TWeakObjectPtr<ThisClass> WeakThis(this);
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis](float)
	{
		if (ThisClass* Self = WeakThis.Get())
		{
			Self->CloseServerWebSocket();
		}
		FPlatformMisc::RequestExit(false);
		return false;
	}), 0.5f);
}

//~ End UFrontendSessionBackend Interface

//~ Begin Client HTTP helpers

void UFrontendSessionBackend_GameLift::SendGameLiftRequest(const FString& TargetAction, const TSharedRef<FJsonObject>& PayloadObject, FGameLiftJsonCallback Callback)
{
	const UFrontendGameLiftSettings* Settings = UFrontendGameLiftSettings::Get();
	const FString Region = Settings->ResolveAwsRegion();
	const FString ProxyUrl = Settings->ResolveApiEndpointUrl();

	FString SerializedPayload;
	{
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&SerializedPayload);
		FJsonSerializer::Serialize(PayloadObject, Writer);
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetContentAsString(SerializedPayload);

	if (!ProxyUrl.IsEmpty())
	{
		HttpRequest->SetURL(ProxyUrl);
		HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/x-amz-json-1.1"));
		HttpRequest->SetHeader(TEXT("X-Amz-Target"), TargetAction);
	}
	else
	{
		FString AccessKey;
		FString SecretKey;
		FString SessionToken;
		if (!Settings->ResolveAwsCredentials(AccessKey, SecretKey, SessionToken))
		{
			Callback(false, nullptr, LOCTEXT("MissingAwsCredentials",
				"No GameLift API Endpoint URL or AWS credentials found. Set AWS_ACCESS_KEY_ID / AWS_SECRET_ACCESS_KEY (or Config/UserGame.ini) for development, or configure ApiEndpointUrl."));
			return;
		}

		const FString Host = FString::Printf(TEXT("gamelift.%s.amazonaws.com"), *Region);
		HttpRequest->SetURL(FString::Printf(TEXT("https://%s/"), *Host));
		FrontendGameLiftCrypto::SignGameLiftJsonRequest(HttpRequest, Host, Region, TargetAction, SerializedPayload, AccessKey, SecretKey, SessionToken);
	}

	ActiveHttpRequest = HttpRequest;
	TWeakObjectPtr<ThisClass> WeakThis(this);
	HttpRequest->OnProcessRequestComplete().BindLambda(
		[WeakThis, TargetAction, Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			ThisClass* Self = WeakThis.Get();
			if (Self && Self->ActiveHttpRequest == Request)
			{
				Self->ActiveHttpRequest.Reset();
			}

			if (!bConnectedSuccessfully || !Response.IsValid())
			{
				if (Self)
				{
					Self->LogDebug(FString::Printf(TEXT("[gamelift] %s: no response (network error)"), *TargetAction));
				}
				Callback(false, nullptr, FText::Format(LOCTEXT("NetworkErrorFmt", "Could not reach GameLift endpoint for {0}."), FText::FromString(TargetAction)));
				return;
			}

			const int32 StatusCode = Response->GetResponseCode();
			const FString ResponseBody = Response->GetContentAsString();

			TSharedPtr<FJsonObject> ParsedJson;
			{
				TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
				FJsonSerializer::Deserialize(Reader, ParsedJson);
			}

			if (StatusCode < 200 || StatusCode >= 300)
			{
				FString ErrorMessage;
				FString ErrorType;
				if (ParsedJson.IsValid())
				{
					ParsedJson->TryGetStringField(TEXT("message"), ErrorMessage);
					if (ErrorMessage.IsEmpty())
					{
						ParsedJson->TryGetStringField(TEXT("Message"), ErrorMessage);
					}
					ParsedJson->TryGetStringField(TEXT("__type"), ErrorType);
				}
				if (ErrorMessage.IsEmpty())
				{
					ErrorMessage = ResponseBody.Left(240);
				}

				if (Self)
				{
					Self->LogDebug(FString::Printf(TEXT("[gamelift] %s failed: HTTP %d %s %s"), *TargetAction, StatusCode, *ErrorType, *ErrorMessage));
				}
				Callback(false, ParsedJson, FText::Format(LOCTEXT("GameLiftApiErrorFmt", "GameLift error ({0}): {1}"), FText::AsNumber(StatusCode), FText::FromString(ErrorMessage)));
				return;
			}

			Callback(true, ParsedJson, FText::GetEmpty());
		});

	HttpRequest->ProcessRequest();
}

void UFrontendSessionBackend_GameLift::CancelPendingClientRequests()
{
	if (ActivationPollTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ActivationPollTickerHandle);
		ActivationPollTickerHandle.Reset();
	}
	PendingActivationGameSessionId.Reset();
	PendingActivationDisplayName.Reset();
	PendingActivationMaxPlayers = 0;
	ActivationPollDeadlineSeconds = 0.0;
	bActivationPollRequestInFlight = false;

	if (ActiveHttpRequest.IsValid())
	{
		ActiveHttpRequest->CancelRequest();
		ActiveHttpRequest.Reset();
	}
}

FString UFrontendSessionBackend_GameLift::BuildLocalPlayerId() const
{
	if (const UWorld* World = GetGameWorld())
	{
		if (const APlayerController* PC = World->GetFirstPlayerController())
		{
			if (const APlayerState* PS = PC->PlayerState)
			{
				const FString Name = PS->GetPlayerName();
				if (!Name.IsEmpty())
				{
					return Name;
				}
			}
		}
	}
	const FString OsUser = FPlatformProcess::UserName(false);
	return OsUser.IsEmpty() ? FString(TEXT("Player")) : OsUser;
}

UWorld* UFrontendSessionBackend_GameLift::GetGameWorld() const
{
	const UFrontendMultiplayerSubsystem* Subsystem = GetOwningSubsystem();
	const UGameInstance* GameInstance = Subsystem ? Subsystem->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetWorld() : nullptr;
}

//~ End Client HTTP helpers

//~ Begin Server WebSocket helpers (GameLift Server SDK 5.x wire protocol)

void UFrontendSessionBackend_GameLift::StartServerWebSocketIfConfigured(UWorld* LoadedWorld)
{
	if (!LoadedWorld || LoadedWorld->GetNetMode() != NM_DedicatedServer)
	{
		return;
	}

	if (LoadedWorld->URL.Port > 0)
	{
		ServerListeningPort = LoadedWorld->URL.Port;
	}

	// One socket per process: valid means connecting or connected. A reconnect after a
	// failure is scheduled by the error/close handlers, not by another world load.
	if (ServerWebSocket.IsValid() || ServerReconnectTickerHandle.IsValid())
	{
		return;
	}

	const UFrontendGameLiftSettings* Settings = UFrontendGameLiftSettings::Get();
	const FString WebSocketUrl = Settings->ResolveServerWebSocketUrl();
	if (WebSocketUrl.IsEmpty())
	{
		LogDebug(TEXT("[gamelift] dedicated server has no GAMELIFT_SDK_WEBSOCKET_URL; running without GameLift"));
		return;
	}

	ServerFleetId = Settings->ResolveFleetId();
	ServerHostId = Settings->ResolveServerHostId();
	if (ServerProcessId.IsEmpty())
	{
		// GameLift identifies this process by pID for its whole life; never regenerate it.
		ServerProcessId = Settings->ResolveServerProcessId();
	}
	const FString AuthToken = Settings->ResolveServerAuthToken();

	if (ServerFleetId.IsEmpty() || ServerHostId.IsEmpty())
	{
		LogDebug(TEXT("[gamelift] WebSocket URL is set but GAMELIFT_SDK_FLEET_ID or GAMELIFT_SDK_HOST_ID is missing"));
		return;
	}

	ServerReconnectAttempts = 0;
	ConnectServerWebSocket(WebSocketUrl, AuthToken);
}

void UFrontendSessionBackend_GameLift::ConnectServerWebSocket(const FString& WebSocketUrl, const FString& AuthToken)
{
	// Release the socket parked by the previous close (we are never inside *that* one's
	// delegate here), then park the current one if any.
	RetiredServerWebSocket.Reset();
	CloseServerWebSocket();

	FModuleManager::Get().LoadModuleChecked<FWebSocketsModule>(TEXT("WebSockets"));

	// Normalise to scheme://host[:port]/path. The engine's WebSocket client takes everything
	// up to the first '/' after the scheme as the host name, so a bare
	// "wss://host?query" would try to resolve "host?query" and fail at once.
	FString BaseUrl = WebSocketUrl.TrimStartAndEnd();
	FString Query;
	{
		int32 QueryIdx = INDEX_NONE;
		if (BaseUrl.FindChar(TEXT('?'), QueryIdx))
		{
			Query = BaseUrl.Mid(QueryIdx + 1);
			BaseUrl.LeftInline(QueryIdx);
		}
	}
	{
		const int32 SchemeEnd = BaseUrl.Find(TEXT("://"));
		const int32 HostStart = SchemeEnd == INDEX_NONE ? 0 : SchemeEnd + 3;
		if (BaseUrl.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, HostStart) == INDEX_NONE)
		{
			BaseUrl += TEXT("/");
		}
	}

	FString FullUrl = FString::Printf(
		TEXT("%s?%s%spID=%s&sdkVersion=5.1.2&sdkLanguage=Cpp&ComputeId=%s&FleetId=%s&IdempotencyToken=%s"),
		*BaseUrl,
		*Query,
		Query.IsEmpty() ? TEXT("") : TEXT("&"),
		*FGenericPlatformHttp::UrlEncode(ServerProcessId),
		*FGenericPlatformHttp::UrlEncode(ServerHostId),
		*FGenericPlatformHttp::UrlEncode(ServerFleetId),
		*FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower));

	if (!AuthToken.IsEmpty())
	{
		FullUrl += FString::Printf(TEXT("&Authorization=%s"), *FGenericPlatformHttp::UrlEncode(AuthToken));
	}

	LogDebug(FString::Printf(TEXT("[gamelift] connecting server WebSocket %s (fleet=%s compute=%s process=%s port=%d%s)"),
		*BaseUrl, *ServerFleetId, *ServerHostId, *ServerProcessId, ServerListeningPort,
		AuthToken.IsEmpty() ? TEXT(", NO auth token") : TEXT("")));

	ServerWebSocket = FWebSocketsModule::Get().CreateWebSocket(FullUrl);
	ServerWebSocket->OnConnected().AddUObject(this, &ThisClass::HandleWebSocketConnected);
	ServerWebSocket->OnConnectionError().AddUObject(this, &ThisClass::HandleWebSocketConnectionError);
	ServerWebSocket->OnClosed().AddUObject(this, &ThisClass::HandleWebSocketClosed);
	ServerWebSocket->OnMessage().AddUObject(this, &ThisClass::HandleWebSocketMessage);
	ServerWebSocket->Connect();
}

void UFrontendSessionBackend_GameLift::CloseServerWebSocket()
{
	if (ServerHeartbeatTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ServerHeartbeatTickerHandle);
		ServerHeartbeatTickerHandle.Reset();
	}
	if (ServerReconnectTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ServerReconnectTickerHandle);
		ServerReconnectTickerHandle.Reset();
	}
	if (ServerSessionEmptyExitTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ServerSessionEmptyExitTickerHandle);
		ServerSessionEmptyExitTickerHandle.Reset();
	}

	bServerProcessReady = false;

	if (ServerWebSocket.IsValid())
	{
		ServerWebSocket->OnConnected().Clear();
		ServerWebSocket->OnConnectionError().Clear();
		ServerWebSocket->OnClosed().Clear();
		ServerWebSocket->OnMessage().Clear();
		if (ServerWebSocket->IsConnected())
		{
			ServerWebSocket->Close();
		}
		// This may run from inside the socket's own delegate (RefreshConnection,
		// TerminateProcess, error/close handlers): park it instead of destroying it here.
		RetiredServerWebSocket = MoveTemp(ServerWebSocket);
		ServerWebSocket.Reset();
	}
}

void UFrontendSessionBackend_GameLift::ScheduleServerReconnect()
{
	constexpr int32 MaxAttempts = 30;
	if (ServerReconnectTickerHandle.IsValid())
	{
		return;
	}
	if (ServerReconnectAttempts >= MaxAttempts)
	{
		LogDebug(FString::Printf(TEXT("[gamelift] giving up on the server WebSocket after %d attempts; restart the server with a fresh GAMELIFT_SDK_AUTH_TOKEN"), ServerReconnectAttempts));
		return;
	}

	++ServerReconnectAttempts;
	const float DelaySeconds = FMath::Min(5.f * ServerReconnectAttempts, 60.f);
	LogDebug(FString::Printf(TEXT("[gamelift] retrying server WebSocket in %.0fs (attempt %d)"), DelaySeconds, ServerReconnectAttempts));
	ServerReconnectTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &ThisClass::HandleServerReconnectTick), DelaySeconds);
}

bool UFrontendSessionBackend_GameLift::HandleServerReconnectTick(float /*DeltaTime*/)
{
	ServerReconnectTickerHandle.Reset();
	const UFrontendGameLiftSettings* Settings = UFrontendGameLiftSettings::Get();
	ConnectServerWebSocket(Settings->ResolveServerWebSocketUrl(), Settings->ResolveServerAuthToken());
	return false; // one-shot
}

void UFrontendSessionBackend_GameLift::SendServerWebSocketMessage(const TSharedRef<FJsonObject>& MessageObject)
{
	if (!ServerWebSocket.IsValid() || !ServerWebSocket->IsConnected())
	{
		return;
	}

	FString Serialized;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Serialized);
	FJsonSerializer::Serialize(MessageObject, Writer);
	ServerWebSocket->Send(Serialized);
}

bool UFrontendSessionBackend_GameLift::HandleServerHeartbeatTick(float /*DeltaTime*/)
{
	if (!ServerWebSocket.IsValid() || !ServerWebSocket->IsConnected() || !bServerProcessReady)
	{
		return true;
	}

	TSharedRef<FJsonObject> HeartbeatMsg = MakeShared<FJsonObject>();
	HeartbeatMsg->SetStringField(TEXT("Action"), TEXT("HeartbeatServerProcess"));
	HeartbeatMsg->SetStringField(TEXT("RequestId"), FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower));
	HeartbeatMsg->SetBoolField(TEXT("HealthStatus"), true);
	SendServerWebSocketMessage(HeartbeatMsg);
	return true;
}

void UFrontendSessionBackend_GameLift::HandleWebSocketConnected()
{
	bServerProcessReady = true;
	ServerReconnectAttempts = 0;

	TSharedRef<FJsonObject> ActivateMsg = MakeShared<FJsonObject>();
	ActivateMsg->SetStringField(TEXT("Action"), TEXT("ActivateServerProcess"));
	ActivateMsg->SetStringField(TEXT("RequestId"), FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower));
	ActivateMsg->SetStringField(TEXT("SdkVersion"), TEXT("5.1.2"));
	ActivateMsg->SetStringField(TEXT("SdkLanguage"), TEXT("Cpp"));
	ActivateMsg->SetNumberField(TEXT("Port"), ServerListeningPort);

	TArray<TSharedPtr<FJsonValue>> LogPaths;
	LogPaths.Add(MakeShared<FJsonValueString>(FPaths::ConvertRelativePathToFull(FPaths::ProjectLogDir())));
	ActivateMsg->SetArrayField(TEXT("LogPaths"), LogPaths);

	SendServerWebSocketMessage(ActivateMsg);

	LogDebug(FString::Printf(TEXT("[gamelift] server WebSocket connected -> ActivateServerProcess (port %d)"), ServerListeningPort));

	if (ServerHeartbeatTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ServerHeartbeatTickerHandle);
	}
	ServerHeartbeatTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &ThisClass::HandleServerHeartbeatTick), 55.0f);
}

void UFrontendSessionBackend_GameLift::HandleWebSocketConnectionError(const FString& Error)
{
	LogDebug(FString::Printf(TEXT("[gamelift] server WebSocket connection error: %s"), *Error));
	CloseServerWebSocket();
	ScheduleServerReconnect();
}

void UFrontendSessionBackend_GameLift::HandleWebSocketClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
	LogDebug(FString::Printf(TEXT("[gamelift] server WebSocket closed (code %d, clean=%s): %s"),
		StatusCode, bWasClean ? TEXT("true") : TEXT("false"), *Reason));
	// GameLift closes the socket itself only around RefreshConnection / TerminateProcess,
	// which are handled in HandleWebSocketMessage; anything else is worth retrying.
	CloseServerWebSocket();
	ScheduleServerReconnect();
}

void UFrontendSessionBackend_GameLift::HandleWebSocketMessage(const FString& MessageString)
{
	TSharedPtr<FJsonObject> JsonObj;
	{
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(MessageString);
		if (!FJsonSerializer::Deserialize(Reader, JsonObj) || !JsonObj.IsValid())
		{
			return;
		}
	}

	FString Action;
	JsonObj->TryGetStringField(TEXT("Action"), Action);
	if (Action.IsEmpty())
	{
		return;
	}

	if (Action.Equals(TEXT("CreateGameSession"), ESearchCase::IgnoreCase))
	{
		FString GameSessionId;
		FString GameSessionName;
		int32 MaximumPlayers = 8;
		JsonObj->TryGetStringField(TEXT("GameSessionId"), GameSessionId);
		JsonObj->TryGetStringField(TEXT("GameSessionName"), GameSessionName);
		JsonObj->TryGetNumberField(TEXT("MaximumPlayerSessionCount"), MaximumPlayers);

		ServerGameSessionId = GameSessionId;
		ActiveGameSessionId = GameSessionId;
		ActiveSessionDisplayName = GameSessionName.IsEmpty() ? FString(TEXT("GameLift Match")) : GameSessionName;
		ActiveSessionMaxPlayers = FMath::Max(1, MaximumPlayers);

		LogDebug(FString::Printf(TEXT("[gamelift] CreateGameSession received (%s '%s' %d players) -> ActivateGameSession"),
			*ServerGameSessionId, *ActiveSessionDisplayName, ActiveSessionMaxPlayers));

		TSharedRef<FJsonObject> ActivateSessionMsg = MakeShared<FJsonObject>();
		ActivateSessionMsg->SetStringField(TEXT("Action"), TEXT("ActivateGameSession"));
		ActivateSessionMsg->SetStringField(TEXT("RequestId"), FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower));
		ActivateSessionMsg->SetStringField(TEXT("GameSessionId"), ServerGameSessionId);
		SendServerWebSocketMessage(ActivateSessionMsg);

		ReportServerSessionInfoUpdated();
	}
	else if (Action.Equals(TEXT("RefreshConnection"), ESearchCase::IgnoreCase))
	{
		FString NewEndpoint;
		FString NewAuthToken;
		JsonObj->TryGetStringField(TEXT("RefreshConnectionEndpoint"), NewEndpoint);
		JsonObj->TryGetStringField(TEXT("AuthToken"), NewAuthToken);

		if (!NewEndpoint.IsEmpty())
		{
			LogDebug(TEXT("[gamelift] RefreshConnection received -> reconnecting"));
			ConnectServerWebSocket(NewEndpoint, NewAuthToken);
		}
	}
	else if (Action.Equals(TEXT("TerminateProcess"), ESearchCase::IgnoreCase))
	{
		TerminateServerProcessAndExit(TEXT("TerminateProcess received from GameLift"));
	}
	else
	{
		// Replies to our own requests (ActivateServerProcess, Heartbeat, ActivateGameSession...)
		// come back with the same Action plus StatusCode / ErrorMessage. Surface failures.
		int32 StatusCode = 200;
		FString ErrorMessage;
		JsonObj->TryGetNumberField(TEXT("StatusCode"), StatusCode);
		JsonObj->TryGetStringField(TEXT("ErrorMessage"), ErrorMessage);
		if (StatusCode != 200 || !ErrorMessage.IsEmpty())
		{
			LogDebug(FString::Printf(TEXT("[gamelift] %s rejected by GameLift (status %d): %s"), *Action, StatusCode, *ErrorMessage));
		}
		else if (Action.Equals(TEXT("ActivateServerProcess"), ESearchCase::IgnoreCase))
		{
			LogDebug(TEXT("[gamelift] ActivateServerProcess acknowledged; process is ACTIVE on the fleet"));
		}
	}
}

//~ End Server WebSocket helpers

#undef LOCTEXT_NAMESPACE
