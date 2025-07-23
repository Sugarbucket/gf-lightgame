#include<graphics.h>
#include<string>
#include<vector>		// 引入vector头文件,替代C语言的数组
#define PI 3.14159		// 定义圆周率常量


int WINDOW_WIDTH = 1280;
int WINDOW_HEIGHT = 720;

const int BUTTON_WIDTH = 400;		// 按钮宽度常量
const int BUTTON_HEIGHT = 100;		// 按钮高度常量

int idx_current_anim = 0;		// 当前动画帧索引

#pragma comment(lib,"MSIMG32.LIB")	// 链接MSIMG32库
#pragma comment(lib, "winmm.lib")

bool is_game_started = false;		// 游戏是否开始的标志
bool running = true;		// 游戏运行状态标志

inline void putimage_alpha(int x, int y, IMAGE* img)		//封装一个透明绘图函数，处理PNG图像黑边问题
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
		TCHAR path_file[256];		// 定义一个字符数组用于存储文件路径
		for (size_t i = 0; i < num; i++)
		{
			_stprintf_s(path_file, path, i);

			IMAGE* frame = new IMAGE();		// 创建一个新的IMAGE对象
			loadimage(frame, path_file);
			frame_list.push_back(frame);		// 将图片对象的指针添加到vector容器中
		}
	}
	~Atlas()
	{
		for (size_t i = 0; i < frame_list.size(); i++)
		{
			delete frame_list[i];		// 释放每个IMAGE对象的内存
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
		interval_ms = interval;		// 设置动画帧间隔时间
	}

	~Animation() = default;

	void AniPlay(int x, int y, int delta)		// 动画播放函数，传入x,y坐标和delta时间,将动画计数器变为动画计时器
	{
		timer += delta;
		if (timer >= interval_ms)
		{
			idx_frame = (idx_frame + 1) % anim_atlas->frame_list.size();
			timer = 0;
		}
		putimage_alpha(x, y, anim_atlas->frame_list[idx_frame]);	// 绘制当前动画帧
	}

private:
	int timer = 0;		// 动画计时器
	int interval_ms = 0;		// 动画帧间隔时间
	int idx_frame = 0;		// 当前动画帧索引
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
		loadimage(&img_SBullet, _T("img/SBullet.png"));		// 加载子弹图片

		int dx = target_pos.x - start_pos.x;		// 计算目标位置与起始位置的水平距离
		int dy = target_pos.y - start_pos.y;		// 计算目标位置与起始位置的垂直距离
		double len = sqrt(dx * dx + dy * dy);		// 计算距离
		if (len > 0)		// 如果距离不为0
		{
			dir_x = dx / len;		// 计算水平方向的单位向量
			dir_y = dy / len;		// 计算垂直方向的单位向量
		}
		else
		{
			dir_x = 0;
			dir_y = 0;
		}
	}

	void Move()
	{
		if (!active)return;		// 如果子弹处于激活状态
		
			pos.x += (int)(dir_x * speed);		// 更新子弹位置
			pos.y += (int)(dir_y * speed);
			if (pos.x < -50 || pos.x > 1280+50 || pos.y < -50 || pos.y > 720+50)	// 如果子弹超出窗口范围
			{
				active = false;		// 设置为非激活状态
			}
		
	}

	void Draw() const
	{
		if (active)		// 如果子弹处于激活状态
		{
			putimage_alpha(pos.x, pos.y, &img_SBullet);		// 绘制子弹
		}
	}

	bool IsActive()const { return active; }
	POINT GetPosition() const { return pos; }		// 获取子弹位置
	void Deactivate() { active = false; }		// 设置子弹为非激活状态
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
		int dir_x = is_move_right - is_move_left;		// 计算水平移动方向
		int dir_y = is_move_down - is_move_up;		// 计算垂直移动方向
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);	// 计算移动方向的长度
		if (len_dir != 0)		// 如果有移动
		{
			double normalized_x = dir_x / len_dir;		// 归一化水平方向
			double normalized_y = dir_y / len_dir;		// 归一化垂直方向
			player_pos.x += normalized_x * PLAYER_SPEED;	// 更新玩家位置
			player_pos.y += normalized_y * PLAYER_SPEED;	// 更新玩家位置
		}

		if (player_pos.x < 0) player_pos.x = 0;
		if (player_pos.y < 0) player_pos.y = 0;
		if (player_pos.x + PLAYER_WIDTH > WINDOW_WIDTH) player_pos.x = WINDOW_WIDTH - PLAYER_WIDTH;
		if (player_pos.y + PLAYER_HEIGHT > WINDOW_HEIGHT) player_pos.y = WINDOW_HEIGHT - PLAYER_HEIGHT;

	}

	void Draw(int delta)
	{
		static bool facing_left = false;		// 玩家是否面向左侧的标志
		int dir_x = is_move_right - is_move_left;		// 计算水平移动方向
		if (dir_x < 0)
			facing_left = true;		// 如果玩家向左移动，设置为面向左侧
		else if (dir_x > 0)
			facing_left = false;	// 如果玩家向右移动，设置为面向右侧

		if (facing_left)		// 如果玩家面向左侧
		{
			anim_left->AniPlay(player_pos.x, player_pos.y, delta);	// 播放左侧动画
		}
		else		// 如果玩家面向右侧
		{
			anim_right->AniPlay(player_pos.x, player_pos.y, delta);;	// 播放右侧动画
		}
	}
	const POINT& getPosition() const
	{
		return player_pos;		// 返回玩家位置
	}


	void Shoot(POINT target_pos)
	{
		DWORD current_time = GetTickCount();		// 获取当前时间
		if (current_time - last_shoot_time < SHOOT_COOLDOWN)return;		// 如果距离上次射击时间超过冷却时间

		POINT center=
		{
			player_pos.x + PLAYER_WIDTH / 2,
			player_pos.y + PLAYER_HEIGHT / 2
		};		// 计算玩家中心位置
		sbullets.push_back(new SBullet(center, target_pos));		// 创建新的子弹并添加到列表中
		last_shoot_time = current_time;		// 更新上次射击时间
	}

	void UpdateSBullet()
	{
		auto it = sbullets.begin();		// 创建迭代器指向子弹列表的起始位置
		while (it != sbullets.end())		// 遍历子弹列表
		{
			(*it)->Move();		// 移动子弹
			if (!(*it)->IsActive())		// 如果子弹不再激活
			{
				delete* it;		// 释放子弹内存
				it = sbullets.erase(it);		// 从列表中删除子弹并更新迭代器
			}
			else
			{
				++it;		// 移动到下一个子弹
			}
		}
	}
	void DrawSBullet() const
		{
			for (const auto& bullet : sbullets)		// 遍历子弹列表
			{
				bullet->Draw();		// 绘制子弹
			}
		}

	const std::vector <SBullet*>& GetBullets() const
		{
			return sbullets;		// 返回子弹列表
		}

private:
	const int PLAYER_WIDTH = 80;			// 玩家宽度常量
	const int PLAYER_HEIGHT = 80;			// 玩家高度常量
	const int PLAYER_SPEED = 4;			// 玩家移动速度常量
private:
	Animation* anim_left;
	Animation* anim_right;
	POINT player_pos = { 500,500 };		// 玩家位置
	bool is_move_up = false;		// 是否向上移动
	bool is_move_down = false;		// 是否向下移动
	bool is_move_left = false;		// 是否向左移动
	bool is_move_right = false;		// 是否向右移动


	std::vector <SBullet*>sbullets;
	const int SHOOT_COOLDOWN = 500;
	DWORD last_shoot_time = 0;		// 上次射击时间


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
	static const int FRAME_WIDTH = 80;			// 敌人宽度常量
	static const int FRAME_HEIGHT = 100;			// 敌人高度常量
	const POINT& getPosition() const
	{
		return position;
	}
public:
	Enemy()
	{
		anim_left = new Animation(atlas_enemy_left,100);	// 左侧敌人动画对象
		anim_right = new Animation(atlas_enemy_right,100);	// 右侧敌人动画对象

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
		case SpawnEdge::LEFT:		// 如果生成在左侧
			position.x = 0;		// 设置x坐标为0
			position.y = rand() % (WINDOW_HEIGHT - ENEMY_HEIGHT);	// 随机y坐标
			break;
		case SpawnEdge::RIGHT:		// 如果生成在右侧
			position.x = WINDOW_WIDTH - ENEMY_WIDTH;	// 设置x坐标为窗口宽度减去敌人宽度
			position.y = rand() % (WINDOW_HEIGHT - ENEMY_HEIGHT);	// 随机y坐标
			break;
		case SpawnEdge::TOP:		// 如果生成在顶部
			position.x = rand() % (WINDOW_WIDTH - ENEMY_WIDTH);	// 随机x坐标
			position.y = 0;		// 设置y坐标为0
			break;
		case SpawnEdge::BOTTOM:		// 如果生成在底部
			position.x = rand() % (WINDOW_WIDTH - ENEMY_WIDTH);	// 随机x坐标
			position.y = WINDOW_HEIGHT - ENEMY_HEIGHT;	// 设置y坐标为窗口高度减去敌人高度
			break;
		default:
			break;		// 默认情况不做任何操作
		}
	}

	~Enemy()
	{
		delete anim_left;		// 释放左侧敌人动画对象
		delete anim_right;		// 释放右侧敌人动画对象
	}

	bool CheckPlayerCollision(const Player& player) {
		POINT checkpoint = {
			position.x + Player::FRAME_WIDTH / 2,
			position.y + Player::FRAME_HEIGHT / 2
		};
		bool is_overlap_x = checkpoint.x >= player.getPosition().x && checkpoint.x <= player.getPosition().x + Player::FRAME_WIDTH;  //检测水平方向重叠
		bool is_overlap_y = checkpoint.y >= player.getPosition().y && checkpoint.y <= player.getPosition().y + Player::FRAME_HEIGHT;  //检测垂直方向重叠
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
		const POINT& player_pos = player.getPosition();		// 获取玩家位置
		int dir_x = player_pos.x - position.x;		// 计算水平方向
		int dir_y = player_pos.y - position.y;		// 计算垂直方向
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);	// 计算移动方向的长度
		if (len_dir != 0)		// 如果有移动
		{
			double normalized_x = dir_x / len_dir;		// 归一化水平方向
			double normalized_y = dir_y / len_dir;		// 归一化垂直方向
			position.x += (int)(normalized_x * ENEMY_SPEED);	// 更新敌人位置
			position.y += (int)(normalized_y * ENEMY_SPEED);	// 更新敌人位置
		}
		if (dir_x < 0)
			facing_left = true;
		else if (dir_x > 0)
			facing_left = false;

	}

	void Draw(int delta)
	{

		if (facing_left)		// 如果敌人面向左侧
			anim_left->AniPlay(position.x, position.y, delta);	// 播放左侧动画
		else		// 如果敌人面向右侧
			anim_right->AniPlay(position.x, position.y, delta);	// 播放右侧动画
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
	POINT position = { 0,0 };		// 敌人位置
	bool facing_left = false;
	bool alive = true;		// 敌人是否存活的标志

private:
	const int ENEMY_WIDTH = 80;			// 敌人宽度常量
	const int ENEMY_HEIGHT = 100;			// 敌人高度常量
	const int ENEMY_SPEED = 3;			// 敌人移动速度常量
};

class Button
{
public:
	Button(RECT rect,LPCTSTR path_img_idle,LPCTSTR path_img_hovered,LPCTSTR path_img_pushed)
	{
		region = rect;
		loadimage(&img_idle, path_img_idle);		// 加载空闲状态图像
		loadimage(&img_hovered, path_img_hovered);		// 加载悬停状态图像
		loadimage(&img_pushed, path_img_pushed);		// 加载按下状态图像

	}
	~Button() = default;
	
	void ProcessEvent(const ExMessage& msg)
	{
		switch (msg.message)
		{
		case WM_MOUSEMOVE:
			if(status==Status::Idle&&CheckCursorHit(msg.x, msg.y))		// 如果鼠标悬停在按钮上
			{
				status = Status::Hovered;		// 切换到悬停状态
			}
			else if(status==Status::Hovered && !CheckCursorHit(msg.x, msg.y))		// 如果鼠标离开按钮区域
			{
				status = Status::Idle;		// 切换回空闲状态
			}
			break;
		case WM_LBUTTONDOWN:
			if(CheckCursorHit(msg.x, msg.y))		// 如果鼠标点击在按钮区域
			{
				status = Status::Pushed;		// 切换到按下状态
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
			putimage_alpha(region.left, region.top, &img_idle);		// 绘制空闲状态图像
			break;
		case Status::Hovered:
			putimage_alpha(region.left, region.top, &img_hovered);		// 绘制悬停状态图像
			break;
		case Status::Pushed:
			putimage_alpha(region.left, region.top, &img_pushed);		// 绘制按下状态图像
			break;
		}
	}

protected:
	virtual void OnClick() = 0;		// 按钮点击事件处理函数，子类需要实现此函数来定义按钮被点击时的行为

private:
	enum class Status
	{
		Idle=0,		// 空闲状态
		Hovered,	// 悬停状态
		Pushed,		// 按下状态
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
		return x >= region.left && x <= region.right && y >= region.top && y <= region.bottom;		// 检查鼠标位置是否在按钮区域内
	}
};

class Crosshair
{
public:
	Crosshair()
	{
		// 加载准星图像
		loadimage(&img_crosshair, _T("img/crosshair.png")); // 准备一个准星图片
	}

	void Update()
	{
		// 获取鼠标位置
		GetCursorPos(&position);
		ScreenToClient(GetHWnd(), &position); // 转换为窗口坐标
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
		is_game_started = true;		// 设置游戏开始标志为true
		mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);		// 播放背景音乐
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
		running = false;		// 退出游戏
	}
};

void TryGenerateEnemy(std::vector<Enemy*>& enemy_list)
{
	const int INTERVAL = 100;
	static int counter = 0;
	if (++counter % INTERVAL == 0)		// 每隔一定时间生成一个敌人
	{
		enemy_list.push_back(new Enemy());		// 将敌人添加到容器中
	}
}

void  DrawPlayerScore(int score)
{
	static TCHAR text[64];
	_stprintf_s(text, _T("Score: %d"), score);		// 格式化分数文本
	setbkmode(TRANSPARENT);
	settextcolor(RGB(255, 85, 185));
	outtextxy(10, 10, text);		// 在窗口左上角绘制分数文本
}

int main()
{
	initgraph(WINDOW_WIDTH ,WINDOW_HEIGHT);

	atlas_player_left = new Atlas(_T("img/player/player_left_%d.png"), 14);
	atlas_player_right = new Atlas(_T("img/player/player_right_%d.png"), 14);
	atlas_enemy_left = new Atlas(_T("img/enemy/enemy_left_%d.png"), 7);
	atlas_enemy_right = new Atlas(_T("img/enemy/enemy_right_%d.png"), 7);


	mciSendString(_T("open music/bgm.mp3 alias bgm"), NULL, 0, NULL);		// 打开背景音乐
	mciSendString(_T("setaudio bgm volume to 200"), NULL, 0, NULL);
	mciSendString(_T("open music/hit.mp3 alias hit"), NULL, 0, NULL);		// 打开击中音效

	int score = 0;

	Player player;		// 创建玩家对象
	ExMessage msg;
	IMAGE img_background;
	IMAGE img_menu;
	Crosshair crosshair;

	std::vector<Enemy*> enemy_list;		// 创建敌人容器

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

	loadimage(&img_menu, _T("img/menu.png"));		//加载菜单背景图像
	loadimage(&img_background, _T("img/bg.png"));		//加载背景图像
	
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
				if (msg.message == WM_LBUTTONDOWN)		// 如果鼠标左键按下
				{
					player.Shoot({ msg.x,msg.y });		// 玩家开火
				}
			}
		else
		{
			btn_start_game.ProcessEvent(msg);		// 处理开始游戏按钮事件
			btn_quit_game.ProcessEvent(msg);		// 处理退出游戏按钮事件
		}
		}

		if (is_game_started)
		{
			player.Move();		// 更新玩家位置
			player.UpdateSBullet();		// 更新玩家子弹
			TryGenerateEnemy(enemy_list);
			for (Enemy* enemy : enemy_list)
				enemy->Move(player);


			for (Enemy* enemy : enemy_list)
			{
				if (enemy->CheckPlayerCollision(player))
				{
					static TCHAR text[128];
					_stprintf_s(text, _T("You are dead! Score: %d"), score);
					MessageBox(GetHWnd(), _T("wasted"), _T("游戏结束"), MB_OK);
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
						mciSendString(_T("play hit from 0"), NULL, 0, NULL);		// 播放击中音效
						enemy->Hurt();
						score++;
					}
				}
			}

			for (size_t i = 0; i < enemy_list.size(); i++)
			{
				Enemy* enemy = enemy_list[i];
				if (!enemy->CheckAlive())		// 如果敌人不存活
				{
					std::swap(enemy_list[i], enemy_list.back());		// 将其与最后一个敌人交换	
					enemy_list.pop_back();		// 移除最后一个敌人
					delete enemy;		// 释放敌人对象的内存
				}
			}
		}
		cleardevice();
		if (is_game_started)
		{
			putimage(0, 0, &img_background);
			player.Draw(1000 / 144);
			player.DrawSBullet();		// 绘制玩家子弹
			for (Enemy* enemy : enemy_list)
				enemy->Draw(1000 / 144);		// 绘制所有敌人
			DrawPlayerScore(score);		// 绘制玩家分数

		}

		else
		{
			putimage(0, 0, &img_menu);		// 绘制菜单背景
			btn_start_game.Draw();		// 绘制开始游戏按钮
			btn_quit_game.Draw();		// 绘制退出游戏按钮
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