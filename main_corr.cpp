#include <iostream>
#include <thread>
#include <ncurses.h>
#include <chrono>
#include <ctime>
#include <unistd.h>
#include <mutex>
#include <condition_variable>
#include <string>

#pragma region variables
int key=0;
int ammountOfThreads=10;
std::mutex mtx;
std::mutex ch_m;
int missed=0;
bool lineOccupancy[ 26 ];
std::condition_variable service_rdy;
std::condition_variable lines[26];
#pragma endregion

#pragma region steering
void key_list(){
	while (key!=27){
		key=int(getchar());
		//std::cout<<key;
	}
}

void refresh_display(){
	while (key!=27){
		mtx.lock();
		refresh();
		mtx.unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}
}
#pragma endregion

//Wir brauchen die nächste Linie zu prüfen, ob sie frei ist
void falling_sign(int input, char ch, int64_t delay){

	srand(time(NULL)); //Nur für Cygwin
	std::thread::id thread_id = std::this_thread::get_id();
	int start_x = rand()%59+1;
	int start_y = 1;
	int end_y = 25;
	int ch_int = static_cast<int> (ch);
	
	while(key!=27){
		int i=1;
		
		//Die Bewegung der Zeichen
		for(i; i<=end_y; i++){
			std::unique_lock<std::mutex> locker(mtx);
			lines[i].wait(locker, [&](){return lineOccupancy[i]==false;});
			lineOccupancy[i-1]=false;
			mvprintw(i-1,start_x," ");
			lines[i-1].notify_one();
			mvprintw(i,start_x,"%c", ch);
			lineOccupancy[i]=true;
			locker.unlock();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(delay)); //ein Zeichen wartet hier
			if(key==ch_int)	{
				std::unique_lock<std::mutex> locker(mtx);
				mvprintw(i,start_x," "); //aufräumen
				lineOccupancy[i]=false;
				lines[i].notify_one();
				locker.unlock();
				return; 
			}
		}
		mtx.lock();
		lineOccupancy[25]=false;
		lines[25].notify_one();
		missed++;
		mtx.unlock();
	}
}


int main(int argc, char const *argv[]){
	initscr();
	start_color();
	curs_set(0);
	srand(time(NULL));

	std::thread refr(refresh_display);
	std::thread ext(key_list);

	//Infos für den Player
	mvprintw(1,25, "Welcome to typing tutor");
	mvprintw(2,21, "Difficulty level: %d characters", ammountOfThreads);
	sleep(3);
	clear();
	mvprintw(1,31, "Let's begin!");
	sleep(1);
	clear();

	for(int i=0; i<=26; i++) lineOccupancy[i]=false;
	
	//Starte das Spiel
	std::thread * threads = new std::thread[ammountOfThreads];
    for(int i = 0;i < ammountOfThreads;i++)
    {
        char c = '!' + rand()%93+1;
		//srand(time(NULL)); //Nur für Cygwin
		int64_t delay = static_cast<int64_t> (rand()%400+201);
        threads[i] = std::thread(falling_sign,i,c, delay);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); //Neues Zeichen jede Sekunde
    }

	//aufräumen
	for(int i = 0;i < ammountOfThreads;i++)
    {
        threads[i].join();
    }
	delete[] threads;

	//Ende
	clear();
	mtx.lock();
	mvprintw(15,25,"Missed characters: %d", missed);
	mvprintw(16,25,"Press ESC to leave...");
	mtx.unlock();

	refr.join();
	ext.join();

	endwin();
	return 0;
}