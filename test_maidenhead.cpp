#include <iostream>
#include <cmath>
#include <cstring>

char locator[11];

char letterize(int x)
{
    if (x < 24)
        return (char)x + 65;
    else
        return (char)23 + 65;
}

void update_mh_10(double lat, double lon)
{
    // Factors per Maidenhead precision step
    // Index 0: field (letters), 1: square (digits), 2: subsquare (letters),
    // 3: extended square (digits), 4: extended subsquare (digits for 9th/10th)
    double LON_F[] = {20, 2.0, 0.0833330, 0.008333, 0.0008333};
    double LAT_F[] = {10, 1.0, 0.0416665, 0.004166, 0.0004166};

    lon += 180;
    lat += 90;

    // Produce 10-character locator (positions 9 and 10 are digits 0-9)
    const int size = 10;
    for (int i = 0; i < size / 2; i++)
    {
        bool useDigits = (i % 2 == 1) || (i == 4); // digits for 3rd, 4th and 5th pairs
        if (useDigits)
        {
            locator[i * 2]     = (char)((int)(lon / LON_F[i]) + '0');
            locator[i * 2 + 1] = (char)((int)(lat / LAT_F[i]) + '0');
        }
        else
        {
            locator[i * 2]     = letterize((int)(lon / LON_F[i]));
            locator[i * 2 + 1] = letterize((int)(lat / LAT_F[i]));
        }
        lon = fmod(lon, LON_F[i]);
        lat = fmod(lat, LAT_F[i]);
    }
    locator[size] = 0; // null terminator
}

int main()
{
    // Test case 1: IO90WX9166 (Maidenhead, My house)
    double lat1 = 50.965188;
    double lon1 = -0.086390;
    update_mh_10(lat1, lon1);
    std::cout << "Test 1: lat=" << lat1 << " lon=" << lon1 << std::endl;
    std::cout << "Locator: " << locator << std::endl;
    std::cout << "Length: " << strlen(locator) << std::endl << std::endl;

    // Test case 2: CM87TS3975 (Maidenhead, San Francisco Pier)
    double lat2 = 37.789592;
    double lon2 = -122.385272;
    update_mh_10(lat2, lon2);
    std::cout << "Test 2: lat=" << lat2 << " lon=" << lon2 << std::endl;
    std::cout << "Locator: " << locator << std::endl;
    std::cout << "Length: " << strlen(locator) << std::endl << std::endl;

    return 0;
}
