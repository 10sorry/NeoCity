// Fill out your copyright notice in the Description page of Project Settings.


#include "CPP_WeatherFetcher.h"

UCPP_WeatherFetcher::UCPP_WeatherFetcher()
{
    UE_LOG(LogTemp, Log, TEXT("CTOR CALLED"));
    FetchWeatherData();
}

void UCPP_WeatherFetcher::FetchWeatherData()
{

	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    
	Request->OnProcessRequestComplete().BindUObject(this, &UCPP_WeatherFetcher::OnResponseReceived);
    URL = FString::Format(TEXT("https://api.openweathermap.org/data/2.5/weather?q={0}&appid={1}&units={2}"), 
        {City, APIKey , Units});
    Request->SetURL(URL);
	Request->SetVerb("GET");
	Request->SetHeader("Content-Type", "application/json");
	Request->ProcessRequest();
}

void UCPP_WeatherFetcher::SetCity(const FString& NewCity)
{
    City = NewCity;
}

void UCPP_WeatherFetcher::SetAPIKey(const FString& NewKey)
{
    APIKey = NewKey;
}

void UCPP_WeatherFetcher::SetUnits(const FString& NewUnits)
{
    Units = NewUnits;
}

FString UCPP_WeatherFetcher::GetCity() const
{
    return City;
}

FString UCPP_WeatherFetcher::GetAPIKey() const
{
    return APIKey;
}

FString UCPP_WeatherFetcher::GetUnits() const
{
    return Units;
}

float UCPP_WeatherFetcher::GetTemperature() const
{
    return Temperature;
}

float UCPP_WeatherFetcher::GetFeelsLike() const
{
    return FeelsLike;
}

float UCPP_WeatherFetcher::GetHumidity() const
{
    return Humidity;
}

float UCPP_WeatherFetcher::GetPressure() const
{
    return Pressure;
}

float UCPP_WeatherFetcher::GetWindSpeed() const
{
    return WindSpeed;
}

FString UCPP_WeatherFetcher::GetWeatherDescription() const
{
    return WeatherDescription;
}

void UCPP_WeatherFetcher::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	 if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("HTTP Request failed"));
        return;
    }
    
    int32 ResponseCode = Response->GetResponseCode();
    
    if (ResponseCode != 200)
    {
        UE_LOG(LogTemp, Error, TEXT("HTTP Error: %d"), ResponseCode);
        return;
    }
    
    FString ResponseContent = Response->GetContentAsString();
    
    // Парсинг JSON
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON"));
        return;
    }
    
    // Извлекаем основные данные о погоде
    const TSharedPtr<FJsonObject>* MainObject;
    if (JsonObject->TryGetObjectField(TEXT("main"), MainObject))
    {
        Temperature = (*MainObject)->GetNumberField(TEXT("temp"));
        FeelsLike = (*MainObject)->GetNumberField(TEXT("feels_like"));
        Humidity = (*MainObject)->GetNumberField(TEXT("humidity"));
        Pressure = (*MainObject)->GetNumberField(TEXT("pressure"));
        
        FString TempUnit = (Units == TEXT("metric")) ? TEXT("°C") : 
                          (Units == TEXT("imperial")) ? TEXT("°F") : TEXT("K");
        
        UE_LOG(LogTemp, Log, TEXT("Temperature: %.1f%s (Feels like: %.1f%s)"), 
            Temperature, *TempUnit, FeelsLike, *TempUnit);
        UE_LOG(LogTemp, Log, TEXT("Humidity: %.0f%%, Pressure: %.0f hPa"), 
            Humidity, Pressure);
    }
    
    // Извлекаем данные о ветре
    const TSharedPtr<FJsonObject>* WindObject;
    if (JsonObject->TryGetObjectField(TEXT("wind"), WindObject))
    {
        WindSpeed = (*WindObject)->GetNumberField(TEXT("speed"));
        UE_LOG(LogTemp, Log, TEXT("Wind Speed: %.1f m/s"), WindSpeed);
    }
    
    // Извлекаем описание погоды
    const TArray<TSharedPtr<FJsonValue>>* WeatherArray;
    if (JsonObject->TryGetArrayField(TEXT("weather"), WeatherArray) && WeatherArray->Num() > 0)
    {
        TSharedPtr<FJsonObject> WeatherObj = (*WeatherArray)[0]->AsObject();
        WeatherDescription = WeatherObj->GetStringField(TEXT("description"));
        FString Main = WeatherObj->GetStringField(TEXT("main"));
        
        UE_LOG(LogTemp, Log, TEXT("Weather: %s (%s)"), *Main, *WeatherDescription);
    }
    
    // Извлекаем название города
    if (JsonObject->TryGetStringField(TEXT("name"), CityName))
    {
        UE_LOG(LogTemp, Log, TEXT("City: %s"), *CityName);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Weather data updated successfully!"));
    OnWeatherUpdated.Broadcast();
}
