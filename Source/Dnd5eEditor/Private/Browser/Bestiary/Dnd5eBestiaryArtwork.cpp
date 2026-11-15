// Developed by Neko Creative Technologies

#include "Browser/Bestiary/Dnd5eBestiaryArtwork.h"

#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"

#if PLATFORM_WINDOWS
#include "Misc/ScopeExit.h"
#include "Templates/RefCounting.h"
#include "Windows/WindowsHWrapper.h"
THIRD_PARTY_INCLUDES_START
#include <wincodec.h>
THIRD_PARTY_INCLUDES_END
#endif

namespace
{
	bool IsBestiaryArtworkFileAvailable(const FString& Path)
	{
		const int64 Bytes = IFileManager::Get().FileSize(*Path);
		return Bytes > 0 && Bytes <= 16 * 1024 * 1024;
	}

#if PLATFORM_WINDOWS
	UTexture2D* LoadBestiaryWebP(const FString& Path)
	{
		const HRESULT Apartment = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		if (FAILED(Apartment) && Apartment != RPC_E_CHANGED_MODE) { return nullptr; }
		ON_SCOPE_EXIT
		{
			if (SUCCEEDED(Apartment)) { CoUninitialize(); }
		};

		TRefCountPtr<IWICImagingFactory> Factory;
		if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(Factory.GetInitReference()))))
		{
			return nullptr;
		}

		TRefCountPtr<IWICBitmapDecoder> Decoder;
		TRefCountPtr<IWICBitmapFrameDecode> Frame;
		if (FAILED(Factory->CreateDecoderFromFilename(*Path, nullptr, GENERIC_READ,
			WICDecodeMetadataCacheOnDemand, Decoder.GetInitReference())) ||
			FAILED(Decoder->GetFrame(0, Frame.GetInitReference())))
		{
			return nullptr;
		}

		UINT Width = 0;
		UINT Height = 0;
		constexpr UINT MaxDimension = 4096;
		if (FAILED(Frame->GetSize(&Width, &Height)) ||
			Width == 0 || Height == 0 || Width > MaxDimension || Height > MaxDimension)
		{
			return nullptr;
		}

		TRefCountPtr<IWICFormatConverter> Converter;
		if (FAILED(Factory->CreateFormatConverter(Converter.GetInitReference())) ||
			FAILED(Converter->Initialize(Frame.GetReference(), GUID_WICPixelFormat32bppBGRA,
				WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)))
		{
			return nullptr;
		}

		const UINT Stride = Width * 4;
		const UINT Bytes = Stride * Height;
		TArray64<uint8> Pixels;
		Pixels.SetNumUninitialized(Bytes);
		if (FAILED(Converter->CopyPixels(nullptr, Stride, Bytes, Pixels.GetData()))) { return nullptr; }

		UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8, NAME_None, Pixels);
		if (Texture)
		{
			Texture->SRGB = true;
			Texture->UpdateResource();
		}
		return Texture;
	}
#endif
}

UTexture2D* Dnd5eBestiaryArtwork::LoadTexture(const FString& Path)
{
	const FString Extension = FPaths::GetExtension(Path).ToLower();
	if (Extension == TEXT("webp"))
	{
#if PLATFORM_WINDOWS
		if (IsBestiaryArtworkFileAvailable(Path))
		{
			if (UTexture2D* Texture = LoadBestiaryWebP(Path)) { return Texture; }
		}
#endif
		const FString PngPath = FPaths::ChangeExtension(Path, TEXT("png"));
		return IsBestiaryArtworkFileAvailable(PngPath) ? FImageUtils::ImportFileAsTexture2D(PngPath) : nullptr;
	}

	if (Extension != TEXT("png") && Extension != TEXT("jpg") && Extension != TEXT("jpeg")) { return nullptr; }
	return IsBestiaryArtworkFileAvailable(Path) ? FImageUtils::ImportFileAsTexture2D(Path) : nullptr;
}
