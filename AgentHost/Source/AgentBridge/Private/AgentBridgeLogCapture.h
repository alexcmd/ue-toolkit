// Copyright Anthropic, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/OutputDevice.h"
#include "Misc/ScopeLock.h"
#include "Misc/DateTime.h"

/** A single captured log entry. */
struct FAgentBridgeLogEntry
{
	FDateTime Timestamp;
	FString Category;
	ELogVerbosity::Type Verbosity;
	FString Message;
};

/**
 * Custom FOutputDevice that captures log output into a thread-safe ring buffer.
 * Register with GLog->AddOutputDevice() and unregister with GLog->RemoveOutputDevice().
 */
class FAgentBridgeLogCapture : public FOutputDevice
{
public:
	explicit FAgentBridgeLogCapture(int32 InMaxEntries = 2000)
		: MaxEntries(InMaxEntries)
	{
		Buffer.Reserve(MaxEntries);
	}

	virtual ~FAgentBridgeLogCapture() override = default;

	/** Called by the logging system from any thread. */
	virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override
	{
		FScopeLock Lock(&CriticalSection);

		FAgentBridgeLogEntry Entry;
		Entry.Timestamp = FDateTime::UtcNow();
		Entry.Category = Category.ToString();
		Entry.Verbosity = Verbosity;
		Entry.Message = Message;

		if (Buffer.Num() < MaxEntries)
		{
			Buffer.Add(MoveTemp(Entry));
		}
		else
		{
			Buffer[WriteIndex] = MoveTemp(Entry);
		}
		WriteIndex = (WriteIndex + 1) % MaxEntries;
		TotalWritten++;
	}

	/**
	 * Get recent log entries, optionally filtered.
	 * @param Count      Max entries to return (0 = all available)
	 * @param Filter     Substring to match in message (empty = no filter)
	 * @param MinSeverity Minimum verbosity: "error", "warning", "log"/"display", "all"/""
	 */
	TArray<FAgentBridgeLogEntry> GetRecentEntries(int32 Count, const FString& Filter, const FString& MinSeverity) const
	{
		ELogVerbosity::Type MinVerbosity = ParseSeverity(MinSeverity);

		FScopeLock Lock(&CriticalSection);

		// Collect entries in chronological order
		TArray<FAgentBridgeLogEntry> Ordered;
		const int32 NumStored = FMath::Min(Buffer.Num(), MaxEntries);

		if (TotalWritten <= MaxEntries)
		{
			// Buffer hasn't wrapped yet
			Ordered = Buffer;
		}
		else
		{
			// Buffer has wrapped — read from WriteIndex to end, then start to WriteIndex
			Ordered.Reserve(NumStored);
			for (int32 i = 0; i < NumStored; ++i)
			{
				Ordered.Add(Buffer[(WriteIndex + i) % MaxEntries]);
			}
		}

		// Filter
		TArray<FAgentBridgeLogEntry> Filtered;
		for (const FAgentBridgeLogEntry& Entry : Ordered)
		{
			if (Entry.Verbosity > MinVerbosity)
			{
				continue;
			}
			if (!Filter.IsEmpty() && !Entry.Message.Contains(Filter))
			{
				continue;
			}
			Filtered.Add(Entry);
		}

		// Return last Count entries
		if (Count > 0 && Filtered.Num() > Count)
		{
			return TArray<FAgentBridgeLogEntry>(Filtered.GetData() + (Filtered.Num() - Count), Count);
		}
		return Filtered;
	}

	/** Convert verbosity enum to string. */
	static FString VerbosityToString(ELogVerbosity::Type Verbosity)
	{
		switch (Verbosity)
		{
		case ELogVerbosity::Fatal:       return TEXT("fatal");
		case ELogVerbosity::Error:       return TEXT("error");
		case ELogVerbosity::Warning:     return TEXT("warning");
		case ELogVerbosity::Display:     return TEXT("display");
		case ELogVerbosity::Log:         return TEXT("log");
		case ELogVerbosity::Verbose:     return TEXT("verbose");
		case ELogVerbosity::VeryVerbose: return TEXT("very_verbose");
		default:                         return TEXT("unknown");
		}
	}

private:
	static ELogVerbosity::Type ParseSeverity(const FString& Severity)
	{
		if (Severity.Equals(TEXT("error"), ESearchCase::IgnoreCase))
		{
			return ELogVerbosity::Error;
		}
		if (Severity.Equals(TEXT("warning"), ESearchCase::IgnoreCase))
		{
			return ELogVerbosity::Warning;
		}
		if (Severity.Equals(TEXT("log"), ESearchCase::IgnoreCase) || Severity.Equals(TEXT("display"), ESearchCase::IgnoreCase))
		{
			return ELogVerbosity::Display;
		}
		// "all" or empty → return VeryVerbose to include everything
		return ELogVerbosity::VeryVerbose;
	}

	mutable FCriticalSection CriticalSection;
	TArray<FAgentBridgeLogEntry> Buffer;
	int32 MaxEntries;
	int32 WriteIndex = 0;
	int64 TotalWritten = 0;
};
