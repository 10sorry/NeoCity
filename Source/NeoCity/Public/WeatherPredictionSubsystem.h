#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Dom/JsonObject.h"
#include "WeatherPredictionSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FWeatherSample
{
    GENERATED_BODY()

    // YYYY-MM-DD or epoch
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Date;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double TemperatureC = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double Pressure = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double Humidity = 0.0;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double Precipitation = 0.0;
};

USTRUCT(BlueprintType)
struct FTrendResult
{
    GENERATED_BODY()

    // slope in degrees C per sample-day
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    double Slope = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    double Intercept = 0.0;

    // mean temperature
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    double Mean = 0.0;
};

UCLASS(Blueprintable)
class UWeatherPredictionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "WeatherPrediction")
    void AddSample(const FWeatherSample& Sample);

    UFUNCTION(BlueprintCallable, Category = "WeatherPrediction")
    void ClearHistory();

    UFUNCTION(BlueprintCallable, Category = "WeatherPrediction")
    TArray<FWeatherSample> GetHistory() const { return History; }
    
    UFUNCTION(BlueprintCallable, Category = "WeatherPrediction")
    FTrendResult ComputeTemperatureTrend(int32 DaysToUse = 30) const;
    
    UFUNCTION(BlueprintCallable, Category = "WeatherPrediction")
    float ComputeStabilityIndex(int32 DaysToUse = 30) const;
    
    UFUNCTION(BlueprintCallable, Category = "WeatherPrediction")
    float GetAdaptiveUpdateIntervalSeconds(int32 DaysToUse = 30, float BaseIntervalSeconds = 600.0f) const;
    
    UFUNCTION(BlueprintCallable, Category = "WeatherPrediction")
    TArray<double> ForecastTemperatureDays(int32 DaysAhead = 3, int32 DaysToUse = 30) const;

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void LoadHistoryFromDisk();
    void SaveHistoryToDisk() const;
    
    TArray<FWeatherSample> GetLatestSamples(int32 DaysToUse) const;

private:
    FString HistoryFilePath;
    
    UPROPERTY()
    TArray<FWeatherSample> History;
};
