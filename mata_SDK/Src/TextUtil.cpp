#include "TextUtil.h"
#include "CameraUtil.h"
#include "TransformUtil.h"
#include "ComputeUtil.h"
#include "StringUtil.h"

void TextUtil::Init(const wchar_t* FontName, int Type, int Italic) {
	hDC = wglGetCurrentDC();
	FontBase = glGenLists(65536);

	Font = CreateFont(
		-1, 0, 0, 0, Type, Italic, FALSE, FALSE, DEFAULT_CHARSET, OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS,
		NONANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, FontName
	);

	LineLengthBuffer.reserve(20);
}

void TextUtil::Reset(int RenderTypeFlag) {
	RenderType = RenderTypeFlag;
	TextAlign = ALIGN_DEFAULT;
	HeightAlign = HEIGHT_ALIGN_DEFAULT;
	FixMiddleCommand = false;
	ShadowRenderCommand = false;
	TextLineGap = 0.0;
	Rotation = 0.0;
	TextOpacity = 1.0;
}

void TextUtil::SetRenderType(int Type) {
	RenderType = Type;
}

void TextUtil::SetAlign(int AlignOpt) {
	TextAlign = AlignOpt;
}

void TextUtil::SetLineGap(GLfloat Value) {
	TextLineGap = Value;
}

void TextUtil::EnableFixMiddle() {
	FixMiddleCommand = true;
}

void TextUtil::DisableFixMiddle() {
	FixMiddleCommand = false;
}

void TextUtil::SetHeightAlign(int Type) {
	HeightAlign = Type;
}

void TextUtil::EnableShadow() {
	ShadowRenderCommand = true;
}

void TextUtil::DisableShadow() {
	ShadowRenderCommand = false;
}

void TextUtil::SetShadow(GLfloat OffsetX, GLfloat OffsetY, GLfloat Opacity, glm::vec3 Color) {
	ShadowOffset.x = OffsetX;
	ShadowOffset.y = OffsetY;
	ShadowOpacity = Opacity;
	ShadowColor = Color;
}

void TextUtil::SetColor(GLfloat R, GLfloat G, GLfloat B) {
	TextColor.r = R;
	TextColor.g = G;
	TextColor.b = B;
}

void TextUtil::SetColor(glm::vec3& Color) {
	TextColor.r = Color.r;
	TextColor.g = Color.g;
	TextColor.b = Color.b;
}

void TextUtil::SetColorRGB(int R, int G, int B) {
	TextColor.r = (1.0f / 255.0f) * (GLfloat)R;
	TextColor.g = (1.0f / 255.0f) * (GLfloat)G;
	TextColor.b = (1.0f / 255.0f) * (GLfloat)B;
}

void TextUtil::Rotate(GLfloat RotationValue) {
	Rotation = RotationValue;
}

void TextUtil::SetOpacity(GLfloat Value) {
	TextOpacity = Value;
}

void TextUtil::Render(glm::vec2& Position, GLfloat Size, const wchar_t* Fmt, ...) {
	if (Fmt == NULL)
		return;

	va_list Args{};
	va_start(Args, Fmt);
	std::vector<wchar_t> Text(vswprintf(nullptr, 0, Fmt, Args) + 1);
	va_end(Args);

	va_start(Args, Fmt);
	vswprintf(Text.data(), Text.size(), Fmt, Args);
	va_end(Args);

	InputText(Text, Position, Size);
}

void TextUtil::Render(GLfloat X, GLfloat Y, GLfloat Size, const wchar_t* Fmt, ...) {
	if (Fmt == NULL)
		return;

	va_list Args{};
	va_start(Args, Fmt);
	std::vector<wchar_t> Text(vswprintf(nullptr, 0, Fmt, Args) + 1);
	va_end(Args);

	va_start(Args, Fmt);
	vswprintf(Text.data(), Text.size(), Fmt, Args);
	va_end(Args);

	InputText(Text, glm::vec2(X, Y), Size);
}

void TextUtil::RenderStr(glm::vec2& Position, GLfloat Size, std::string Str) {
	Render(Position.x, Position.y, Size, stringUtil.Wstring(Str).c_str());
}

void TextUtil::RenderStr(GLfloat X, GLfloat Y, GLfloat Size, std::string Str) {
	Render(X, Y, Size, stringUtil.Wstring(Str).c_str());
}

void TextUtil::RenderWStr(glm::vec2& Position, GLfloat Size, std::wstring WStr) {
	Render(Position.x, Position.y, Size, WStr.c_str());
}

void TextUtil::RenderWStr(GLfloat X, GLfloat Y, GLfloat Size, std::wstring WStr) {
	Render(X, Y, Size, WStr.c_str());
}

////////////////// private
void TextUtil::InputText(std::vector<wchar_t>& Input, glm::vec2& Position, GLfloat Size) {
	CurrentText = std::wstring(Input.data());

	if (ShadowRenderCommand) {
		RenderColor = ShadowColor;
		RenderOpacity = TextOpacity * ShadowOpacity;
		ProcessText(Input.data(), Position + ShadowOffset, Size);
	}

	RenderColor = TextColor;
	RenderOpacity = TextOpacity;
	ProcessText(Input.data(), Position, Size);
}

void TextUtil::ProcessText(wchar_t* Text, glm::vec2 Position, GLfloat Size) {
	CurrentLine = 0;
	TextRenderSize = Size;
	RenderPosition = Position;
	CurrentRenderPosition = 0.0;

	if (CurrentText != PrevText) {
		TextWordCount = wcslen(Text);
		ProcessGlyphCache(Text);
		PrevText = CurrentText;
	}

	CalculateTextLength(Text);

	switch (HeightAlign) {
	case HEIGHT_ALIGN_MIDDLE:
		RenderPosition.y -= TextRenderSize * 0.5;
		break;

	case HEIGHT_ALIGN_UNDER:
		RenderPosition.y -= TextRenderSize;
		break;
	}

	for (int i = 0; i < TextWordCount; ++i) {
		if (Text[i] == L'\n') {
			NextLine();
			continue;
		}

		TransformText();
		camera.SetCamera(RenderType);
		PrepareRender();

		glPushAttrib(GL_LIST_BIT);
		glListBase(FontBase);
		glCallLists(1, GL_UNSIGNED_SHORT, &Text[i]);
		glPopAttrib();

		unsigned int CharIndex = Text[i];
		if (CharIndex < 65536)
			CurrentRenderPosition += TextGlyph[CharIndex].gmfCellIncX * (TextRenderSize / 1.0f);
	}
}

void TextUtil::GetLineLength(const wchar_t* Text) {
	LineLengthBuffer.clear();
	GLfloat CurrentLineLength{};

	for (int i = 0; i < wcslen(Text); ++i) {
		if (Text[i] == L'\n') {
			LineLengthBuffer.emplace_back(CurrentLineLength);
			CurrentLineLength = 0.0f; 
			continue;
		}

		unsigned int CharIndex = Text[i];
		if (CharIndex < 65536) 
			CurrentLineLength += TextGlyph[CharIndex].gmfCellIncX * (TextRenderSize / 1.0f);
	}

	if (CurrentLineLength > 0.0f)
		LineLengthBuffer.emplace_back(CurrentLineLength);
}

void TextUtil::CalculateTextLength(const wchar_t* Text) {
	GetLineLength(Text);
	TextLength = LineLengthBuffer[0];

	MiddleHeight = 0.0;
	if (FixMiddleCommand) {
		size_t LineNum = LineLengthBuffer.size();
		for (int i = 0; i < LineNum; ++i)
			MiddleHeight += (TextLineGap + TextRenderSize);
		MiddleHeight /= 2.0;
	}
}

void TextUtil::NextLine() {
	RenderPosition.y -= (TextLineGap + TextRenderSize);
	CurrentRenderPosition = 0.0;

	if (TextAlign != ALIGN_DEFAULT) {
		++CurrentLine;
		TextLength = LineLengthBuffer[CurrentLine];
	}
}

void TextUtil::TransformText() {
	transform.Identity(TextMatrix);
	transform.Move(TextMatrix, RenderPosition.x, RenderPosition.y + MiddleHeight);

	switch (TextAlign) {
	case ALIGN_DEFAULT:
		transform.Rotate(TextMatrix, Rotation);
		transform.Move(TextMatrix, CurrentRenderPosition, 0.0);
		break;

	case ALIGN_MIDDLE:
		transform.Move(TextMatrix, -TextLength / 2.0, 0.0);
		transform.Rotate(TextMatrix, Rotation);
		transform.Move(TextMatrix, CurrentRenderPosition, 0.0);
		break;

	case ALIGN_LEFT:
		transform.Move(TextMatrix, -TextLength, 0.0);
		transform.Rotate(TextMatrix, Rotation);
		transform.Move(TextMatrix, CurrentRenderPosition, 0.0);
		break;
	}

	transform.Scale(TextMatrix, TextRenderSize, TextRenderSize);
}

void TextUtil::ProcessGlyphCache(wchar_t* Text) {
	for (int i = 0; i < TextWordCount; ++i) {
		if (!CheckGlyphCache(Text[i]))
			LoadGlyph(Text[i]);
	}
}

bool TextUtil::CheckGlyphCache(wchar_t& Char) {
	return GlyphCache.find(Char) != GlyphCache.end() && GlyphCache[Char];
}

void TextUtil::LoadGlyph(wchar_t& Char) {
	if (Char >= 65536)
		return;

	HFONT OldFont = (HFONT)SelectObject(hDC, Font);
	GLYPHMETRICSFLOAT Glyph;
	wglUseFontOutlinesW(hDC, Char, 1, FontBase + Char, 0.0f, 0.0f, WGL_FONT_POLYGONS, &Glyph);
	TextGlyph[Char] = Glyph;
	SelectObject(hDC, OldFont);
	GlyphCache[Char] = true;
}

void TextUtil::PrepareRender() {
	glUseProgram(TEXT_SHADER);
	camera.PrepareRender(SHADER_TYPE_TEXT);

	glUniform1f(TEXT_OPACITY_LOCATION, RenderOpacity);
	glUniform3f(TEXT_COLOR_LOCATION, RenderColor.r, RenderColor.g, RenderColor.b);
	glUniformMatrix4fv(TEXT_MODEL_LOCATION, 1, GL_FALSE, value_ptr(TextMatrix));
}

TextUtil::~TextUtil() {
	HFONT OldFont = (HFONT)SelectObject(hDC, Font);
	SelectObject(hDC, OldFont);
	DeleteObject(Font);
	glDeleteLists(FontBase, 65536);
	DeleteDC(hDC);
}