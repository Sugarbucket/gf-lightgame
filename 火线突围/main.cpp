#include<graphics.h>
#include<string>
#include<vector>		// ����vectorͷ�ļ�,���C���Ե�����
#define PI 3.14159		// ����Բ���ʳ���


int WINDOW_WIDTH = 1280;
int WINDOW_HEIGHT = 720;

const int BUTTON_WIDTH = 400;		// ��ť��ȳ���
const int BUTTON_HEIGHT = 100;		// ��ť�߶ȳ���

int idx_current_anim = 0;		// ��ǰ����֡����

#pragma comment(lib,"MSIMG32.LIB")	// ����MSIMG32��
#pragma comment(lib, "winmm.lib")

bool is_game_started = false;		// ��Ϸ�Ƿ�ʼ�ı�־
bool running = true;		// ��Ϸ����״̬��־

inline void putimage_alpha(int x, int y, IMAGE* img)		//��װһ��͸����ͼ����������PNGͼ��ڱ�����
{
	int w = img->getwidth();
	int h = img->getheight();
	AlphaBlend(GetImageHDC(NULL), x, y, w, h,
		GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
}

class Atlas
{
public:
	Atlas(LPCTSTR path,int num)
	{
		TCHAR path_file[256];		// ����һ���ַ��������ڴ洢�ļ�·��
		for (size_t i = 0; i < num; i++)
		{
			_stprintf_s(path_file, path, i);

			IMAGE* frame = new IMAGE();		// ����һ���µ�IMAGE����
			loadimage(frame, path_file);
			frame_list.push_back(frame);		// ��ͼƬ�����ָ����ӵ�vector������
		}
	}
	~Atlas()
	{
		for (size_t i = 0; i < frame_list.size(); i++)
		{
			delete frame_list[i];		// �ͷ�ÿ��IMAGE������ڴ�
		}
	}
public:
	std::vector<IMAGE*>frame_list;
};

Atlas* atlas_player_left;
Atlas* atlas_player_right;
Atlas* atlas_enemy_left;
Atlas* atlas_enemy_right;

class Animation
{
public:
	Animation(Atlas*atlas,int interval)
	{
		anim_atlas = atlas;
		interval_ms = interval;		// ���ö���֡���ʱ��
	}

	~Animation() = default;

	void AniPlay(int x, int y, int delta)		// �������ź���������x,y�����deltaʱ��,��������������Ϊ������ʱ��
	{
		timer += delta;
		if (timer >= interval_ms)
		{
			idx_frame = (idx_frame + 1) % anim_atlas->frame_list.size();
			timer = 0;
		}
		putimage_alpha(x, y, anim_atlas->frame_list[idx_frame]);	// ���Ƶ�ǰ����֡
	}

private:
	int timer = 0;		// ������ʱ��
	int interval_ms = 0;		// ����֡���ʱ��
	int idx_frame = 0;		// ��ǰ����֡����
private:
	Atlas* anim_atlas;
};

class SBullet
{
	POINT pos;
	const int speed = 10;
	double dir_x, dir_y;
	bool active = true;
	mutable IMAGE img_SBullet;
public:
	SBullet(POINT start_pos, POINT target_pos) :pos(start_pos)
	{
		loadimage(&img_SBullet, _T("img/SBullet.png"));		// �����ӵ�ͼƬ

		int dx = target_pos.x - start_pos.x;		// ����Ŀ��λ������ʼλ�õ�ˮƽ����
		int dy = target_pos.y - start_pos.y;		// ����Ŀ��λ������ʼλ�õĴ�ֱ����
		double len = sqrt(dx * dx + dy * dy);		// �������
		if (len > 0)		// ������벻Ϊ0
		{
			dir_x = dx / len;		// ����ˮƽ����ĵ�λ����
			dir_y = dy / len;		// ���㴹ֱ����ĵ�λ����
		}
		else
		{
			dir_x = 0;
			dir_y = 0;
		}
	}

	void Move()
	{
		if (!active)return;		// ����ӵ����ڼ���״̬
		
			pos.x += (int)(dir_x * speed);		// �����ӵ�λ��
			pos.y += (int)(dir_y * speed);
			if (pos.x < -50 || pos.x > 1280+50 || pos.y < -50 || pos.y > 720+50)	// ����ӵ��������ڷ�Χ
			{
				active = false;		// ����Ϊ�Ǽ���״̬
			}
		
	}

	void Draw() const
	{
		if (active)		// ����ӵ����ڼ���״̬
		{
			putimage_alpha(pos.x, pos.y, &img_SBullet);		// �����ӵ�
		}
	}

	bool IsActive()const { return active; }
	POINT GetPosition() const { return pos; }		// ��ȡ�ӵ�λ��
	void Deactivate() { active = false; }		// �����ӵ�Ϊ�Ǽ���״̬
};

class Player
{

public:
	static const int FRAME_WIDTH = 80;
	static const int FRAME_HEIGHT = 80;
public:
	Player()
	{
		anim_left = new Animation(atlas_player_left , 100);
		anim_right = new Animation(atlas_player_right, 100);
	}
	~Player()
	{
		delete anim_left;
		delete anim_right;
	}
	void processmessage(const ExMessage& msg)
	{
		switch (msg.message)
		{
		case WM_KEYDOWN:
			switch (msg.vkcode)
			{
			case VK_UP:
				is_move_up = true;
				break;
			case VK_DOWN:
				is_move_down = true;
				break;
			case VK_LEFT:
				is_move_left = true;
				break;
			case VK_RIGHT:
				is_move_right = true;
				break;
			}
			break;
		case WM_KEYUP:
			switch (msg.vkcode)
			{
			case VK_UP:
				is_move_up = false;
				break;
			case VK_DOWN:
				is_move_down = false;
				break;
			case VK_LEFT:
				is_move_left = false;
				break;
			case VK_RIGHT:
				is_move_right = false;
				break;
			}
			break;
		}
	}

	void Move()
	{
		int dir_x = is_move_right - is_move_left;		// ����ˮƽ�ƶ�����
		int dir_y = is_move_down - is_move_up;		// ���㴹ֱ�ƶ�����
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);	// �����ƶ�����ĳ���
		if (len_dir != 0)		// ������ƶ�
		{
			double normalized_x = dir_x / len_dir;		// ��һ��ˮƽ����
			double normalized_y = dir_y / len_dir;		// ��һ����ֱ����
			player_pos.x += normalized_x * PLAYER_SPEED;	// �������λ��
			player_pos.y += normalized_y * PLAYER_SPEED;	// �������λ��
		}

		if (player_pos.x < 0) player_pos.x = 0;
		if (player_pos.y < 0) player_pos.y = 0;
		if (player_pos.x + PLAYER_WIDTH > WINDOW_WIDTH) player_pos.x = WINDOW_WIDTH - PLAYER_WIDTH;
		if (player_pos.y + PLAYER_HEIGHT > WINDOW_HEIGHT) player_pos.y = WINDOW_HEIGHT - PLAYER_HEIGHT;

	}

	void Draw(int delta)
	{
		static bool facing_left = false;		// ����Ƿ��������ı�־
		int dir_x = is_move_right - is_move_left;		// ����ˮƽ�ƶ�����
		if (dir_x < 0)
			facing_left = true;		// �����������ƶ�������Ϊ�������
		else if (dir_x > 0)
			facing_left = false;	// �����������ƶ�������Ϊ�����Ҳ�

		if (facing_left)		// �������������
		{
			anim_left->AniPlay(player_pos.x, player_pos.y, delta);	// ������ද��
		}
		else		// �����������Ҳ�
		{
			anim_right->AniPlay(player_pos.x, player_pos.y, delta);;	// �����Ҳද��
		}
	}
	const POINT& getPosition() const
	{
		return player_pos;		// �������λ��
	}


	void Shoot(POINT target_pos)
	{
		DWORD current_time = GetTickCount();		// ��ȡ��ǰʱ��
		if (current_time - last_shoot_time < SHOOT_COOLDOWN)return;		// ��������ϴ����ʱ�䳬����ȴʱ��

		POINT center=
		{
			player_pos.x + PLAYER_WIDTH / 2,
			player_pos.y + PLAYER_HEIGHT / 2
		};		// �����������λ��
		sbullets.push_back(new SBullet(center, target_pos));		// �����µ��ӵ�����ӵ��б���
		last_shoot_time = current_time;		// �����ϴ����ʱ��
	}

	void UpdateSBullet()
	{
		auto it = sbullets.begin();		// ����������ָ���ӵ��б����ʼλ��
		while (it != sbullets.end())		// �����ӵ��б�
		{
			(*it)->Move();		// �ƶ��ӵ�
			if (!(*it)->IsActive())		// ����ӵ����ټ���
			{
				delete* it;		// �ͷ��ӵ��ڴ�
				it = sbullets.erase(it);		// ���б���ɾ���ӵ������µ�����
			}
			else
			{
				++it;		// �ƶ�����һ���ӵ�
			}
		}
	}
	void DrawSBullet() const
		{
			for (const auto& bullet : sbullets)		// �����ӵ��б�
			{
				bullet->Draw();		// �����ӵ�
			}
		}

	const std::vector <SBullet*>& GetBullets() const
		{
			return sbullets;		// �����ӵ��б�
		}

private:
	const int PLAYER_WIDTH = 80;			// ��ҿ�ȳ���
	const int PLAYER_HEIGHT = 80;			// ��Ҹ߶ȳ���
	const int PLAYER_SPEED = 4;			// ����ƶ��ٶȳ���
private:
	Animation* anim_left;
	Animation* anim_right;
	POINT player_pos = { 500,500 };		// ���λ��
	bool is_move_up = false;		// �Ƿ������ƶ�
	bool is_move_down = false;		// �Ƿ������ƶ�
	bool is_move_left = false;		// �Ƿ������ƶ�
	bool is_move_right = false;		// �Ƿ������ƶ�


	std::vector <SBullet*>sbullets;
	const int SHOOT_COOLDOWN = 500;
	DWORD last_shoot_time = 0;		// �ϴ����ʱ��


};

class Enemy
{
public:
	/*bool CheckBulletCollision(const Bullet& bullet) const
	{
		return bullet.CheckCollision(*this);
	}
	*/
public:
	static const int FRAME_WIDTH = 80;			// ���˿�ȳ���
	static const int FRAME_HEIGHT = 100;			// ���˸߶ȳ���
	const POINT& getPosition() const
	{
		return position;
	}
public:
	Enemy()
	{
		anim_left = new Animation(atlas_enemy_left,100);	// �����˶�������
		anim_right = new Animation(atlas_enemy_right,100);	// �Ҳ���˶�������

		enum class SpawnEdge
		{
			LEFT=0,
			RIGHT,
			TOP,
			BOTTOM
		};

		SpawnEdge edge = (SpawnEdge)(rand() % 4);
		switch (edge)
		{
		case SpawnEdge::LEFT:		// ������������
			position.x = 0;		// ����x����Ϊ0
			position.y = rand() % (WINDOW_HEIGHT - ENEMY_HEIGHT);	// ���y����
			break;
		case SpawnEdge::RIGHT:		// ����������Ҳ�
			position.x = WINDOW_WIDTH - ENEMY_WIDTH;	// ����x����Ϊ���ڿ�ȼ�ȥ���˿��
			position.y = rand() % (WINDOW_HEIGHT - ENEMY_HEIGHT);	// ���y����
			break;
		case SpawnEdge::TOP:		// ��������ڶ���
			position.x = rand() % (WINDOW_WIDTH - ENEMY_WIDTH);	// ���x����
			position.y = 0;		// ����y����Ϊ0
			break;
		case SpawnEdge::BOTTOM:		// ��������ڵײ�
			position.x = rand() % (WINDOW_WIDTH - ENEMY_WIDTH);	// ���x����
			position.y = WINDOW_HEIGHT - ENEMY_HEIGHT;	// ����y����Ϊ���ڸ߶ȼ�ȥ���˸߶�
			break;
		default:
			break;		// Ĭ����������κβ���
		}
	}

	~Enemy()
	{
		delete anim_left;		// �ͷ������˶�������
		delete anim_right;		// �ͷ��Ҳ���˶�������
	}

	bool CheckPlayerCollision(const Player& player) {
		POINT checkpoint = {
			position.x + Player::FRAME_WIDTH / 2,
			position.y + Player::FRAME_HEIGHT / 2
		};
		bool is_overlap_x = checkpoint.x >= player.getPosition().x && checkpoint.x <= player.getPosition().x + Player::FRAME_WIDTH;  //���ˮƽ�����ص�
		bool is_overlap_y = checkpoint.y >= player.getPosition().y && checkpoint.y <= player.getPosition().y + Player::FRAME_HEIGHT;  //��ⴹֱ�����ص�
		return is_overlap_x && is_overlap_y;
	}

	bool CheckSBulletCollision(const SBullet& sbullet)
	{
		bool is_overlap_x = sbullet.GetPosition().x >= position.x && sbullet.GetPosition().x <= position.x + FRAME_WIDTH;
		bool is_overlap_y = sbullet.GetPosition().y >= position.y && sbullet.GetPosition().y <= position.y + FRAME_HEIGHT;
		return is_overlap_x && is_overlap_y;
	}

	void Move(const Player& player)
	{
		const POINT& player_pos = player.getPosition();		// ��ȡ���λ��
		int dir_x = player_pos.x - position.x;		// ����ˮƽ����
		int dir_y = player_pos.y - position.y;		// ���㴹ֱ����
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);	// �����ƶ�����ĳ���
		if (len_dir != 0)		// ������ƶ�
		{
			double normalized_x = dir_x / len_dir;		// ��һ��ˮƽ����
			double normalized_y = dir_y / len_dir;		// ��һ����ֱ����
			position.x += (int)(normalized_x * ENEMY_SPEED);	// ���µ���λ��
			position.y += (int)(normalized_y * ENEMY_SPEED);	// ���µ���λ��
		}
		if (dir_x < 0)
			facing_left = true;
		else if (dir_x > 0)
			facing_left = false;

	}

	void Draw(int delta)
	{

		if (facing_left)		// ��������������
			anim_left->AniPlay(position.x, position.y, delta);	// ������ද��
		else		// ������������Ҳ�
			anim_right->AniPlay(position.x, position.y, delta);	// �����Ҳද��
	}

	void Hurt()
	{
		alive = false;
	}
	bool CheckAlive() const
	{
		return alive;
	}

private:

	Animation* anim_left;
	Animation* anim_right;
	POINT position = { 0,0 };		// ����λ��
	bool facing_left = false;
	bool alive = true;		// �����Ƿ���ı�־

private:
	const int ENEMY_WIDTH = 80;			// ���˿�ȳ���
	const int ENEMY_HEIGHT = 100;			// ���˸߶ȳ���
	const int ENEMY_SPEED = 3;			// �����ƶ��ٶȳ���
};

class Button
{
public:
	Button(RECT rect,LPCTSTR path_img_idle,LPCTSTR path_img_hovered,LPCTSTR path_img_pushed)
	{
		region = rect;
		loadimage(&img_idle, path_img_idle);		// ���ؿ���״̬ͼ��
		loadimage(&img_hovered, path_img_hovered);		// ������ͣ״̬ͼ��
		loadimage(&img_pushed, path_img_pushed);		// ���ذ���״̬ͼ��

	}
	~Button() = default;
	
	void ProcessEvent(const ExMessage& msg)
	{
		switch (msg.message)
		{
		case WM_MOUSEMOVE:
			if(status==Status::Idle&&CheckCursorHit(msg.x, msg.y))		// ��������ͣ�ڰ�ť��
			{
				status = Status::Hovered;		// �л�����ͣ״̬
			}
			else if(status==Status::Hovered && !CheckCursorHit(msg.x, msg.y))		// �������뿪��ť����
			{
				status = Status::Idle;		// �л��ؿ���״̬
			}
			break;
		case WM_LBUTTONDOWN:
			if(CheckCursorHit(msg.x, msg.y))		// ���������ڰ�ť����
			{
				status = Status::Pushed;		// �л�������״̬
			}
			break;
		case WM_LBUTTONUP:
			if (status == Status::Pushed)
			{
				OnClick();
			}
			break;
		default:
			break;
		}
		
	}

	void Draw()
	{
		switch (status)
		{
		case Status::Idle:
			putimage_alpha(region.left, region.top, &img_idle);		// ���ƿ���״̬ͼ��
			break;
		case Status::Hovered:
			putimage_alpha(region.left, region.top, &img_hovered);		// ������ͣ״̬ͼ��
			break;
		case Status::Pushed:
			putimage_alpha(region.left, region.top, &img_pushed);		// ���ư���״̬ͼ��
			break;
		}
	}

protected:
	virtual void OnClick() = 0;		// ��ť����¼���������������Ҫʵ�ִ˺��������尴ť�����ʱ����Ϊ

private:
	enum class Status
	{
		Idle=0,		// ����״̬
		Hovered,	// ��ͣ״̬
		Pushed,		// ����״̬
	};;
private:
	RECT region;
	IMAGE img_idle;
	IMAGE img_hovered;
	IMAGE img_pushed;
	Status status = Status::Idle;
private:
	bool CheckCursorHit(int x, int y)
	{
		return x >= region.left && x <= region.right && y >= region.top && y <= region.bottom;		// ������λ���Ƿ��ڰ�ť������
	}
};

class Crosshair
{
public:
	Crosshair()
	{
		// ����׼��ͼ��
		loadimage(&img_crosshair, _T("img/crosshair.png")); // ׼��һ��׼��ͼƬ
	}

	void Update()
	{
		// ��ȡ���λ��
		GetCursorPos(&position);
		ScreenToClient(GetHWnd(), &position); // ת��Ϊ��������
	}

	void Draw() 
	{
		putimage_alpha(position.x - img_crosshair.getwidth() / 2,position.y - img_crosshair.getheight() / 2,&img_crosshair);
	}

private:
	IMAGE img_crosshair;
	POINT position;
};

class StartGameButton : public Button
{
public:
	StartGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		: Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {
	}
	~StartGameButton() = default;

protected:
	void OnClick() override
	{
		is_game_started = true;		// ������Ϸ��ʼ��־Ϊtrue
		mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);		// ���ű�������
	}
};

class QuitGameButton :public Button
{
	public:
	QuitGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		: Button(rect, path_img_idle, path_img_hovered, path_img_pushed) { }
	~QuitGameButton() = default;
protected:
	void OnClick() override
	{
		running = false;		// �˳���Ϸ
	}
};

void TryGenerateEnemy(std::vector<Enemy*>& enemy_list)
{
	const int INTERVAL = 100;
	static int counter = 0;
	if (++counter % INTERVAL == 0)		// ÿ��һ��ʱ������һ������
	{
		enemy_list.push_back(new Enemy());		// ��������ӵ�������
	}
}

void  DrawPlayerScore(int score)
{
	static TCHAR text[64];
	_stprintf_s(text, _T("Score: %d"), score);		// ��ʽ�������ı�
	setbkmode(TRANSPARENT);
	settextcolor(RGB(255, 85, 185));
	outtextxy(10, 10, text);		// �ڴ������Ͻǻ��Ʒ����ı�
}

int main()
{
	initgraph(WINDOW_WIDTH ,WINDOW_HEIGHT);

	atlas_player_left = new Atlas(_T("img/player/player_left_%d.png"), 14);
	atlas_player_right = new Atlas(_T("img/player/player_right_%d.png"), 14);
	atlas_enemy_left = new Atlas(_T("img/enemy/enemy_left_%d.png"), 7);
	atlas_enemy_right = new Atlas(_T("img/enemy/enemy_right_%d.png"), 7);


	mciSendString(_T("open music/bgm.mp3 alias bgm"), NULL, 0, NULL);		// �򿪱�������
	mciSendString(_T("setaudio bgm volume to 200"), NULL, 0, NULL);
	mciSendString(_T("open music/hit.mp3 alias hit"), NULL, 0, NULL);		// �򿪻�����Ч

	int score = 0;

	Player player;		// ������Ҷ���
	ExMessage msg;
	IMAGE img_background;
	IMAGE img_menu;
	Crosshair crosshair;

	std::vector<Enemy*> enemy_list;		// ������������

	RECT region_btn_start_game, rigion_btn_quit_game;

	region_btn_start_game.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
	region_btn_start_game.right = region_btn_start_game.left + BUTTON_WIDTH;
	region_btn_start_game.top = 430;
	region_btn_start_game.bottom = region_btn_start_game.top + BUTTON_HEIGHT;

	rigion_btn_quit_game.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
	rigion_btn_quit_game.right = rigion_btn_quit_game.left + BUTTON_WIDTH;
	rigion_btn_quit_game.top = 560;
	rigion_btn_quit_game.bottom = rigion_btn_quit_game.top + BUTTON_HEIGHT;

	StartGameButton btn_start_game = StartGameButton(region_btn_start_game, _T("img/button/start_1.png"), _T("img/button/start_3.png"), _T("img/button/start_game_2.png"));
	QuitGameButton btn_quit_game = QuitGameButton(rigion_btn_quit_game, _T("img/button/exit_1.png"), _T("img/button/exit_3.png"), _T("img/button/exit_game_2.png"));

	loadimage(&img_menu, _T("img/menu.png"));		//���ز˵�����ͼ��
	loadimage(&img_background, _T("img/bg.png"));		//���ر���ͼ��
	
	BeginBatchDraw();

	while (running)
	{
		DWORD startTime = GetTickCount();
		crosshair.Update();

		while (peekmessage(&msg))
		{
			if (is_game_started)
			{
				player.processmessage(msg);
				if (msg.message == WM_LBUTTONDOWN)		// �������������
				{
					player.Shoot({ msg.x,msg.y });		// ��ҿ���
				}
			}
		else
		{
			btn_start_game.ProcessEvent(msg);		// ����ʼ��Ϸ��ť�¼�
			btn_quit_game.ProcessEvent(msg);		// �����˳���Ϸ��ť�¼�
		}
		}

		if (is_game_started)
		{
			player.Move();		// �������λ��
			player.UpdateSBullet();		// ��������ӵ�
			TryGenerateEnemy(enemy_list);
			for (Enemy* enemy : enemy_list)
				enemy->Move(player);


			for (Enemy* enemy : enemy_list)
			{
				if (enemy->CheckPlayerCollision(player))
				{
					static TCHAR text[128];
					_stprintf_s(text, _T("You are dead! Score: %d"), score);
					MessageBox(GetHWnd(), _T("wasted"), _T("��Ϸ����"), MB_OK);
					running = false;
					break;
				}
			}


			for(Enemy* enemy : enemy_list)
			{
				for (const SBullet* sbullet : player.GetBullets())
				{
					if (enemy->CheckSBulletCollision(*sbullet))
					{
						mciSendString(_T("play hit from 0"), NULL, 0, NULL);		// ���Ż�����Ч
						enemy->Hurt();
						score++;
					}
				}
			}

			for (size_t i = 0; i < enemy_list.size(); i++)
			{
				Enemy* enemy = enemy_list[i];
				if (!enemy->CheckAlive())		// ������˲����
				{
					std::swap(enemy_list[i], enemy_list.back());		// ���������һ�����˽���	
					enemy_list.pop_back();		// �Ƴ����һ������
					delete enemy;		// �ͷŵ��˶�����ڴ�
				}
			}
		}
		cleardevice();
		if (is_game_started)
		{
			putimage(0, 0, &img_background);
			player.Draw(1000 / 144);
			player.DrawSBullet();		// ��������ӵ�
			for (Enemy* enemy : enemy_list)
				enemy->Draw(1000 / 144);		// �������е���
			DrawPlayerScore(score);		// ������ҷ���

		}

		else
		{
			putimage(0, 0, &img_menu);		// ���Ʋ˵�����
			btn_start_game.Draw();		// ���ƿ�ʼ��Ϸ��ť
			btn_quit_game.Draw();		// �����˳���Ϸ��ť
		}
		crosshair.Draw();
		FlushBatchDraw();

		DWORD endTime = GetTickCount();
		DWORD delta_time=endTime - startTime;
		if (delta_time < 1000 / 144)
		{
			Sleep(1000 / 144 - delta_time);	
		}
	}
	delete atlas_player_left;
	delete atlas_player_right;
	delete atlas_enemy_left;
	delete atlas_enemy_right;
	EndBatchDraw();

	return 0;
}