SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET AUTOCOMMIT = 0;
START TRANSACTION;
SET time_zone = "+00:00";

CREATE TABLE `tw_Accounts`
(
  `UserID` int NOT NULL,
  `Username` varchar(64) NOT NULL,
  `Password` varchar(64) NOT NULL,
  `Language` varchar(64) NOT NULL DEFAULT 'zh-cn',
  `Sword` int NOT NULL DEFAULT 0,
  `Pickaxe` int NOT NULL DEFAULT 0,
  `Axe` int NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

CREATE TABLE `tw_Items`
(
  `UserID` INT NOT NULL,
  `ItemID` INT NOT NULL,
  `Num` INT NOT NULL,
  `Cards` INT NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

ALTER TABLE `tw_Accounts` ADD PRIMARY KEY(`UserID`);

ALTER TABLE `tw_Accounts`
  MODIFY `UserID` int NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=1;
COMMIT;
