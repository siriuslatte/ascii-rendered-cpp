/*
 * Autor: Claudio Ademir Sánchez Barajas
 * Fecha: 11/30/2024
 * 
 * Descripción: Este es un pequeño proyecto que se encarga de renderizar un mapa en 3D, solamente muros y trata de emular
 * un jugador en el espacio. Es bastante primitivo y podria usar mejores tecnicas de renderizado. Pero es un buen punto de partida.
 * 
 * Claro que no hice esto 100% solo, tome de referencia demasiados recursos en internet, pero el codigo fue escrito por mi.
 * 
 * Recursos de los que tome referencia o aprendi algo:
 * - https://tartarus.org/martin/PorterStemmer/
 * - https://es.wikipedia.org/wiki/Wolfenstein_3D
 * - https://es.wikipedia.org/wiki/Ray_casting
 * - https://github.com/ShakedAp/ASCII-renderer
 * - https://es.wikipedia.org/wiki/Píxel
 * - https://learn.microsoft.com/es-es/windows/win32/learnwin32/introduction-to-windows-programming-in-c--
 * - https://learnxinyminutes.com/docs/c++/
 */

#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <chrono>

using namespace std;

#include <stdio.h>
#include <Windows.h>

/*
 * Función que crea un mapa de 16x16 con las siguientes características, este es totalmente modificable a gusto.
*/
static wstring create_map()
{
	wstring map;

	map += L"################";
	map += L"#..............#";
	map += L"#.......########";
	map += L"#..............#";
	map += L"#......####....#";
	map += L"#......####....#";
	map += L"#......####....#";
	map += L"###............#";
	map += L"##.............#";
	map += L"#......#########";
	map += L"#......#.......#";
	map += L"#......#.......#";
	map += L"#..............#";
	map += L"#......#########";
	map += L"#..............#";
	map += L"################";

	return map;
}

int main()
{
	int width = 120;
	int height = 40;

	int mapWidth = 16;
	int mapHeight = 16;

	float player_x = 14.7f;
	float player_y = 5.09f;
	float rotacion = 0.0f;

	const float fov = 3.14159f / 4.0f;
	const float depth = 16.0f;
	const float speed = 5.0f;
	const float step = 0.1f;

	wchar_t* screen = new wchar_t[width * height];

	HANDLE consola;
	{
		consola = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
		SetConsoleActiveScreenBuffer(consola);
	}

	wstring map = create_map();
	DWORD bytesWritten = 0;

	/* 
	 * Variables para el manejo de tiempo, esto nos da una forma de poder
	 * controlar el tiempo que se lleva en cada frame.
	 * 
	 * Tambien nos da la oportunidad de poder controlar mejor el movimiento del jugador
	 * ya que podemos tomar en cuenta los pequeños microsegundos entre frames, asi
	 * se obtiene un movimiento mas suave y controlado (incluso si el usuario tiene un
	 * poco de retraso entre frame, esto es porque al ser ascii characters y estar escribiendo
	 * directamente al buffer de la consola, se puede notar un poco de retraso en el render).
	 */
	auto current_dt = chrono::system_clock::now();
	auto last_dt = chrono::system_clock::now();

	while (true)
	{
		float dt;
		{
			last_dt = chrono::system_clock::now();
			chrono::duration<float> elapsedTime = last_dt - current_dt;

			dt = elapsedTime.count();
		}

		current_dt = last_dt;

		/*
		 * Maneja el movimiento del jugador, asi como su rotación y 
		 * angulo de dirección.
		*/
		{
			if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
				rotacion -= (speed * 0.75f) * dt;

			if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
				rotacion += (speed * 0.75f) * dt;

			if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
			{
				player_x += sinf(rotacion) * speed * dt;
				player_y += cosf(rotacion) * speed * dt;

				if (map.c_str()[(int)player_x * static_cast<std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>>::size_type>(mapWidth) + (int)player_y] == '#')
				{
					player_x -= sinf(rotacion) * speed * dt;
					player_y -= cosf(rotacion) * speed * dt;
				}
			}

			if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
			{
				player_x -= sinf(rotacion) * speed * dt;
				player_y -= cosf(rotacion) * speed * dt;

				if (map.c_str()[(int)player_x * static_cast<std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>>::size_type>(mapWidth) + (int)player_y] == '#')
				{
					player_x += sinf(rotacion) * speed * dt;
					player_y += cosf(rotacion) * speed * dt;
				}
			}
		}

		for (int x = 0; x < width; x++)
		{
			float angulo = (rotacion - fov / 2.0f) + ((float)x / (float)width) * fov;
			float distancia = 0.0f;

			bool hit = false;
			bool limite = false;

			float sx = sinf(angulo);
			float cy = cosf(angulo);

			while (true)
			{
				distancia += step;

				int rx = (int)(player_x + sx * distancia);
				int ry = (int)(player_y + cy * distancia);

				if (rx < 0 || rx >= mapWidth || ry < 0 || ry >= mapHeight)
				{
					hit = true;
					distancia = depth;

					break;
				}
				else
				{
					if (map[static_cast<std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>>::size_type>(rx) * mapWidth + ry] == '#')
					{
						hit = true;

						vector<pair<float, float>> p;

						for (int tx = 0; tx < 2; tx++)
							for (int ty = 0; ty < 2; ty++)
							{
								float vy = (float)ry + ty - player_y;
								float vx = (float)rx + tx - player_x;

								float d = sqrt(vx * vx + vy * vy);
								float dot = (sx * vx / d) + (cy * vy / d);

								p.push_back(make_pair(d, dot));
							}

						// Sortear desde los mas cercas hasta los mas lejos.
						sort(p.begin(), p.end(), [](const pair<float, float>& left, const pair<float, float>& right) {return left.first < right.first; });

						// Fisicamente imposuble ver mas de 4. Asi que simplemente nos hacemos cargo de los primeros 3 casos.
						float fBound = 0.01;

						if (acos(p[0].second) < fBound) limite = true;
						if (acos(p[1].second) < fBound) limite = true;
						if (acos(p[3].second) < fBound) limite = true;

						break;
					}
				}
			}

			int nCeiling = (float)(height / 2.0) - height / distancia;
			int nFloor = height - nCeiling;

			short nShade = ' ';

			// Determinamos que tan lejos esta la pared de la vista del jugador, tenemos varios caracteres Unicode, si desea
			// puede cambiarlos, pero estos fueron de mis referencias.
			{
				if (distancia <= depth / 4.0f) nShade = 0x2588;
				else if (distancia < depth / 3.0f)	nShade = 0x2593;
				else if (distancia < depth / 2.0f) nShade = 0x2592;
				else if (distancia < depth)	nShade = 0x2591;
				else nShade = ' ';

				if (limite)	nShade = ' ';
			}

			for (int y = 0; y < height; y++)
			{
				if (y <= nCeiling)
					screen[y * width + x] = ' ';
				else if (y > nCeiling && y <= nFloor)
					screen[y * width + x] = nShade;
				else
				{
					// Calculamos la sombra basada en la distancia, esto para el piso unicamente.
					float b = 1.0f - (((float)y - height / 2.0f) / ((float)height / 2.0f));

					if (b < 0.25)		nShade = '#';
					else if (b < 0.5)	nShade = 'x';
					else if (b < 0.75)	nShade = '.';
					else if (b < 0.9)	nShade = '-';
					else				nShade = ' ';

					screen[y * width + x] = nShade;
				}
			}
		}

		/*
		 * Formatea la pantalla con la información del jugador, así como los FPS que se llevan.
		 */
		{
			for (int nx = 0; nx < mapWidth; nx++)
				for (int ny = 0; ny < mapWidth; ny++)
				{
					screen[(ny + 1) * width + nx] = map[ny * mapWidth + nx];
				}

			screen[((int)player_x + 1) * width + (int)player_y] = 'P';
		}

		// El ultimo caracter de la pantalla debe ser un null.
		screen[width * height - 1] = '\0';

		// Escribir en la consola, este es el ultimo paso.
		WriteConsoleOutputCharacter(consola, screen, width * height, { 0,0 }, &bytesWritten);
	}

	return 0;
}