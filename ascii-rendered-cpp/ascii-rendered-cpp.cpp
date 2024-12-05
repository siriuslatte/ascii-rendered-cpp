#include <iostream>
#include <cmath>
#include <chrono>

#include <Windows.h>

using namespace std;

/*
 * Crear un mapa para el juego, donde '#' es una pared y '.' es un espacio vacío. 
 * 
 * Puede modificarse perfectamente.
 * 
 * @returns - Un mapa en formato de cadena de texto.
 */
static wstring generate_map()
{
    return L"################"
        L"#..............#"
        L"#.......########"
        L"#..............#"
        L"#......####....#"
        L"#......####....#"
        L"#......####....#"
        L"###............#"
        L"##.............#"
        L"#......#########"
        L"#......#.......#"
        L"#......#.......#"
        L"#..............#"
        L"#......#########"
        L"#..............#"
        L"################";
}

/*
 * Punto de entrada de nuestro programa, como
 * cualquier otro programa de C++.
*/
int main()
{
    // Resolución de pantalla.
    const int width = 120, height = 40;

    // Mapa y posición del jugador.
    const int map_width = 16, map_height = 16;
    float plr_x = 10.0f, plr_y = 5.0f, rot = 0.0f;

    // Parámetros de la cámara, su angulo de visión, profundidad y velocidad.
    const float fov = 3.14159f / 4.0f, depth = 16.0f, speed = 5.0f;

    // Buffer de pantalla.
    wchar_t* screen = new wchar_t[width * height];

    // Crear consola y buffer de pantalla, aqui es en donde apareceran nuestros graficos.
    static HANDLE console;
    {
        console = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
        SetConsoleActiveScreenBuffer(console);
    }

    // Buffer de escritura, esto no se usa, solo se usa por la consola.
    DWORD bytesWritten = 0;

    // Mapa del juego.
    wstring map = generate_map();

    // Tiempo de inicio.
    auto current = chrono::system_clock::now();

    /*
	 * El loop de logica, aqui se controla el movimiento del jugador y 
     * se renderiza el juego.
     * 
     * Usamos scanline rendering como el metodo mas eficiente para poder 
     * tener una aplicación eficiente en la consola. Claro que podriamos añadir
	 * mas o quitar, pero este es un ejemplo básico y nada que vaya a escalar mucho.
     */
    while (true)
    {
		// Calcular el tiempo que hubo entre frames, esto es para que el movimiento sea constante y mas deterministico entre frames.
        auto previous = current;

        current = chrono::system_clock::now();
        chrono::duration<float> elapsedTime = current - previous;

        // Este es el calculo final, delta time es como tal ya el tiempo transcurrido.
        const float dt = elapsedTime.count();

        /*
		 * Controles del jugador, esto nos ayuda a mover al jugador en el mapa.
         */
        {
            const float x_sin = sinf(rot), y_cos = cosf(rot);

			float move_x = x_sin * speed * dt, move_y = y_cos * speed * dt;

            const unsigned short tile = map[static_cast<std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>>::size_type>((plr_x + move_x)) * map_width + (static_cast<unsigned long long>(plr_y) + move_y)];
			const bool hit = tile == '#';

			// Rotar a la izquierda.
            if (GetAsyncKeyState(static_cast<unsigned short>('A')) & 0x8000)
                rot -= speed * 0.75f * dt;

            // Rotar a la derecha.
            if (GetAsyncKeyState(static_cast<unsigned short>('D')) & 0x8000)
                rot += speed * 0.75f * dt;

            // Adelante.
            if (GetAsyncKeyState(static_cast<unsigned short>('W')) & 0x8000)
            {
                // Colisiones con las paredes, ignoremos los casteos de types.
                if (!hit)
                    plr_x += move_x;
                    plr_y += move_y;
                
            }

            // Atras.
            if (GetAsyncKeyState(static_cast<unsigned short>('S')) & 0x8000)
            {
				move_x = -x_sin * speed * dt;
				move_y = -y_cos * speed * dt;

                // Mas colisiones con paredes, pero esto es para atras.
                if (!hit)
                    plr_x += move_x;
                    plr_y += move_y;
            }
        }

        // Renderizar usando Scanline
        for (int x = 0; x < width; x++)
        {
            float angle = (rot - fov / 2.0f) + (static_cast<float>(x) / static_cast<float>(width)) * fov;

            float rx = sinf(angle);
            float ry = cosf(angle);
            float dist = 0.0f;

            while (dist < depth)
            {
                dist += 0.1f;

                int nx = static_cast<int>(plr_x + rx * dist);
                int ny = static_cast<int>(plr_y + ry * dist);

                if (nx < 0 || nx >= map_width || ny < 0 || ny >= map_height)
                {
                    dist = depth;

                    break;
                }
                else if (map[static_cast<std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>>::size_type>(nx) * map_width + ny] == '#')
                {
                    break;
                }
            }

            // Calcular alturas para cielo, pared y piso
            int ceiling = (float)(height / 2.0) - height / dist;
            int floor = height - ceiling;

            for (int y = 0; y < height; y++)
            {
                const int pixel = static_cast<int>(y * width + x);

                if (y < ceiling)
                    screen[pixel] = ' ';
                else if (y > ceiling && y <= floor)
                    screen[pixel] = 0x2588;
                else
                {
                    const float b = 1.0f - (static_cast<float>(y) - height / 2.0f) / (static_cast<float>(height) / 2.0f);

                    char character = (b < 0.25f) ? '#' :
                        (b < 0.5f) ? 'x' :
                        (b < 0.75f) ? '.' :
                        (b < 0.9f) ? '-' : ' ';

                    screen[pixel] = character;

                }
            }
        }

        // Convertir el ultimo en el caracter de escape.
        screen[width * height - 1] = '\0';

		// Renderizar en la consola, esto ya es el paso final en todo el proceso.
        WriteConsoleOutputCharacter(console, screen, width * height, { 0, 0 }, &bytesWritten);
    }

    return 0;
}