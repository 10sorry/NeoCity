// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Http.h"
#include "UObject/NoExportTypes.h"
#include "CPP_WeatherFetcher.generated.h"

/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeatherUpdated);

UCLASS(Blueprintable)
class NEOCITY_API UCPP_WeatherFetcher : public UObject
{
	GENERATED_BODY()
public:
	UCPP_WeatherFetcher();
	UPROPERTY(BlueprintAssignable, Category = "Weather")
	FOnWeatherUpdated OnWeatherUpdated;
	UFUNCTION(BlueprintCallable, Category = "Weather")
	void FetchWeatherData();
	// ========== Setters для настроек ==========
    UFUNCTION(BlueprintCallable, Category = "Weather|Settings")
    void SetCity(const FString& NewCity);
    
    UFUNCTION(BlueprintCallable, Category = "Weather|Settings")
    void SetAPIKey(const FString& NewKey);
    
    UFUNCTION(BlueprintCallable, Category = "Weather|Settings")
    void SetUnits(const FString& NewUnits);
    
    // ========== Getters для настроек ==========
    UFUNCTION(BlueprintPure, Category = "Weather|Settings")
    FString GetCity() const;
    
    UFUNCTION(BlueprintPure, Category = "Weather|Settings")
    FString GetAPIKey() const;
    
    UFUNCTION(BlueprintPure, Category = "Weather|Settings")
    FString GetUnits() const;
    
    // ========== Getters для данных погоды ==========
    UFUNCTION(BlueprintPure, Category = "Weather|Data")
    float GetTemperature() const;
    
    UFUNCTION(BlueprintPure, Category = "Weather|Data")
    float GetFeelsLike() const;
    
    UFUNCTION(BlueprintPure, Category = "Weather|Data")
    float GetHumidity() const;
    
    UFUNCTION(BlueprintPure, Category = "Weather|Data")
    float GetPressure() const;
    
    UFUNCTION(BlueprintPure, Category = "Weather|Data")
    float GetWindSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Weather|Data")
	int GetId() const;

	UFUNCTION(BlueprintPure, Category = "Weather|Data")
	FString GetMain() const;
	
    UFUNCTION(BlueprintPure, Category = "Weather|Data")
    FString GetWeatherDescription() const;

	
private:
	FString City = "Penza";
	FString APIKey = "8b05d62206f459e1d298cbe5844d7d87";
	FString Units = "metric";  // metric, imperial, standard
	FString URL;
    
	// Данные погоды
	float Temperature = 0.0f;
	float FeelsLike = 0.0f;
	float Humidity = 0.0f;
	float Pressure = 0.0f;
	float WindSpeed = 0.0f;
	FString WeatherDescription;
	FString Main;
	int Id;
	FString CityName;
    
	// Callback
	void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
