#include "WeatherPredictionSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Dom/JsonObject.h"
#include "JsonObjectConverter.h"

void UWeatherPredictionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    HistoryFilePath = FPaths::ProjectSavedDir() / TEXT("WeatherHistory.json");
    LoadHistoryFromDisk();
}

void UWeatherPredictionSubsystem::Deinitialize()
{
    SaveHistoryToDisk();
    Super::Deinitialize();
}

void UWeatherPredictionSubsystem::AddSample(const FWeatherSample& Sample)
{
    History.Add(Sample);
    
    const int32 MaxSamples = 365;
    if (History.Num() > MaxSamples)
    {
        int32 Excess = History.Num() - MaxSamples;
        History.RemoveAt(0, Excess, /*bAllowShrinking=*/false);
    }

    SaveHistoryToDisk();
}

void UWeatherPredictionSubsystem::ClearHistory()
{
    History.Reset();
    SaveHistoryToDisk();
}

void UWeatherPredictionSubsystem::LoadHistoryFromDisk()
{
    History.Reset();

    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *HistoryFilePath))
    {
        return;
    }

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("WeatherPrediction: failed to parse history json"));
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>* ArrPtr;
    if (!Root->TryGetArrayField(TEXT("history"), ArrPtr))
    {
        return;
    }

    for (const TSharedPtr<FJsonValue>& V : *ArrPtr)
    {
        TSharedPtr<FJsonObject> Obj = V->AsObject();
        if (!Obj) continue;

        FWeatherSample Sample;
        // ключи: Date, TemperatureC, Pressure, Humidity, Precipitation
        Obj->TryGetStringField(TEXT("Date"), Sample.Date);
        Obj->TryGetNumberField(TEXT("TemperatureC"), Sample.TemperatureC);
        Obj->TryGetNumberField(TEXT("Pressure"), Sample.Pressure);
        Obj->TryGetNumberField(TEXT("Humidity"), Sample.Humidity);
        Obj->TryGetNumberField(TEXT("Precipitation"), Sample.Precipitation);

        History.Add(Sample);
    }
}

void UWeatherPredictionSubsystem::SaveHistoryToDisk() const
{
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Arr;
    for (const FWeatherSample& S : History)
    {
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetStringField(TEXT("Date"), S.Date);
        Obj->SetNumberField(TEXT("TemperatureC"), S.TemperatureC);
        Obj->SetNumberField(TEXT("Pressure"), S.Pressure);
        Obj->SetNumberField(TEXT("Humidity"), S.Humidity);
        Obj->SetNumberField(TEXT("Precipitation"), S.Precipitation);
        Arr.Add(MakeShared<FJsonValueObject>(Obj));
    }
    Root->SetArrayField(TEXT("history"), Arr);

    FString Out;
    TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&Out);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

    FFileHelper::SaveStringToFile(Out, *HistoryFilePath);
}

TArray<FWeatherSample> UWeatherPredictionSubsystem::GetLatestSamples(int32 DaysToUse) const
{
    TArray<FWeatherSample> Copy = History;
    if (DaysToUse <= 0 || Copy.Num() <= DaysToUse)
    {
        return Copy;
    }

    int32 Start = FMath::Max(0, Copy.Num() - DaysToUse);
    TArray<FWeatherSample> Result;
    for (int32 i = Start; i < Copy.Num(); ++i) Result.Add(Copy[i]);
    return Result;
}

FTrendResult UWeatherPredictionSubsystem::ComputeTemperatureTrend(int32 DaysToUse) const
{
    FTrendResult R;
    TArray<FWeatherSample> Samples = GetLatestSamples(DaysToUse);
    int32 N = Samples.Num();
    if (N == 0) return R;

    // x = 0..N-1 , y = temp
    double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumXX = 0.0;
    for (int i = 0; i < N; ++i)
    {
        double x = static_cast<double>(i);
        double y = Samples[i].TemperatureC;
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumXX += x * x;
    }

    double denom = (N * sumXX - sumX * sumX);
    if (FMath::IsNearlyZero(denom))
    {
        R.Slope = 0.0;
        R.Intercept = sumY / N;
    }
    else
    {
        R.Slope = (N * sumXY - sumX * sumY) / denom;
        R.Intercept = (sumY - R.Slope * sumX) / N;
    }

    // mean
    R.Mean = sumY / N;
    return R;
}

float UWeatherPredictionSubsystem::ComputeStabilityIndex(int32 DaysToUse) const
{
    TArray<FWeatherSample> Samples = GetLatestSamples(DaysToUse);
    int32 N = Samples.Num();
    if (N <= 1) return 0.0f;

    // variance of temperature and precipitation (normalized)
    double meanTemp = 0.0;
    double meanPrec = 0.0;
    for (const FWeatherSample& S : Samples)
    {
        meanTemp += S.TemperatureC;
        meanPrec += S.Precipitation;
    }
    meanTemp /= N;
    meanPrec /= N;

    double varTemp = 0.0, varPrec = 0.0;
    for (const FWeatherSample& S : Samples)
    {
        double dt = S.TemperatureC - meanTemp;
        varTemp += dt * dt;
        double dp = S.Precipitation - meanPrec;
        varPrec += dp * dp;
    }
    varTemp /= (N - 1);
    varPrec /= (N - 1);
    
    double tempFactor = FMath::Sqrt(varTemp); // std dev in deg
    double precFactor = FMath::Sqrt(varPrec); // std dev in mm
    
    double normTemp = FMath::Clamp((tempFactor - 0.5) / (5.0 - 0.5), 0.0, 1.0);
    // для осадков: если stddev ~0.1 -> стабильно, >10 -> нестабильно
    double normPrec = FMath::Clamp((precFactor - 0.1) / (10.0 - 0.1), 0.0, 1.0);
    
    double stability = FMath::Clamp(0.6 * normTemp + 0.4 * normPrec, 0.0, 1.0);

    return static_cast<float>(stability);
}

float UWeatherPredictionSubsystem::GetAdaptiveUpdateIntervalSeconds(int32 DaysToUse, float BaseIntervalSeconds) const
{
    float stability = ComputeStabilityIndex(DaysToUse); // 0..1
    float multiplier = FMath::Lerp(0.2f, 2.0f, stability);
    float interval = BaseIntervalSeconds * multiplier;
    interval = FMath::Clamp(interval, 30.0f, 3600.0f * 6.0f); // min 30s, max 6h
    return interval;
}

TArray<double> UWeatherPredictionSubsystem::ForecastTemperatureDays(int32 DaysAhead, int32 DaysToUse) const
{
    TArray<double> Out;
    if (DaysAhead <= 0) return Out;

    FTrendResult Trend = ComputeTemperatureTrend(DaysToUse);
    
    TArray<FWeatherSample> Samples = GetLatestSamples(DaysToUse);
    int N = Samples.Num();
    if (N == 0)
    {
        for (int i = 0; i < DaysAhead; ++i) Out.Add(Trend.Intercept);
        return Out;
    }

    int startX = N;
    for (int i = 0; i < DaysAhead; ++i)
    {
        double x = static_cast<double>(startX + i);
        double pred = Trend.Slope * x + Trend.Intercept;
        Out.Add(pred);
    }
    return Out;
}
